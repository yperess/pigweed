# Copyright 2024 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Tooling to generate C++ constants from a yaml sensor definition."""

import argparse
from dataclasses import dataclass
from collections.abc import Sequence
from enum import Enum, auto
import io
import re
import sys

import yaml


@dataclass(frozen=True)
class Printable:
    """Common printable object"""

    id: str
    name: str
    description: str | None

    @property
    def variable_name(self) -> str:
        """Convert the 'id' to a constant variable name in C++."""
        return "k" + ''.join(
            ele.title() for ele in re.split(r"[\s_-]+", self.id)
        )

    def print(self, writer: io.TextIOWrapper) -> None:
        """
        Baseclass for a printable sensor object

        Args:
          writer: IO writer used to print values.
        """
        writer.write(
            f"""
/// @brief {self.name}
"""
        )
        if self.description:
            writer.write(
                f"""///
/// {self.description}
"""
            )


@dataclass(frozen=True)
class Units(Printable):
    """A single unit representation"""

    symbol: str

    def print(self, writer: io.TextIOWrapper) -> None:
        """
        Print header definition for this unit

        Args:
          writer: IO writer used to print values.
        """
        super().print(writer=writer)
        writer.write(
            f"""PW_SENSOR_UNIT_TYPE(
    {super().variable_name},
    "PW_SENSOR_UNITS_TYPE",
    "{self.name}",
    "{self.symbol}"
);
"""
        )


@dataclass(frozen=True)
class Attribute(Printable):
    """A single attribute representation."""

    def print(self, writer: io.TextIOWrapper) -> None:
        """
        Print header definition for this attribute

        Args:
          writer: IO writer used to print values.
        """
        super().print(writer=writer)
        writer.write(
            f"""PW_SENSOR_ATTRIBUTE_TYPE(
    {super().variable_name},
    "PW_SENSOR_ATTRIBUTE_TYPE",
    "{self.name}"
);
"""
        )


class Representation(Enum):
    """Representation types"""

    SIGNED = auto()
    UNSIGNED = auto()
    FLOAT = auto()

    def __init__(self, *args) -> None:
        super().__init__()
        self.language_map: dict[Representation, str] = {}

    def __str__(self) -> str:
        return self.language_map[self]


@dataclass(frozen=True)
class Channel(Printable):
    """A single channel representation."""

    representation: Representation
    units: Units

    def print(self, writer: io.TextIOWrapper) -> None:
        """
        Print header definition for this channel

        Args:
          writer: IO writer used to print values.
        """
        super().print(writer=writer)
        writer.write(
            f"""PW_SENSOR_MEASUREMENT_TYPE(
    {super().variable_name},
    "PW_SENSOR_MEASUREMENT_TYPE",
    "{self.name}",
    ::pw::sensor::units::{self.units.variable_name},
    {self.representation}
);
"""
        )


@dataclass(frozen=True)
class Trigger(Printable):
    """A single trigger representation."""

    def print(self, writer: io.TextIOWrapper) -> None:
        """
        Print header definition for this trigger

        Args:
          writer: IO writer used to print values.
        """
        super().print(writer=writer)
        writer.write(
            f"""PW_SENSOR_TRIGGER_TYPE(
    {super().variable_name},
    "PW_SENSOR_TRIGGER_TYPE",
    "{self.name}"
);
"""
        )


@dataclass
class Args:
    """CLI arguments"""

    package: Sequence[str]
    language: str


def attribute_from_dict(attribute_id: str, definition: dict) -> Attribute:
    """
    Construct an Attribute from a dictionary entry.

    Args:
      attribute_id: The ID of the attribute in the attribute map
      definition: The node condent of ["attributes"][attribute_id]
    """
    return Attribute(
        id=attribute_id,
        name=definition["name"],
        description=definition["description"],
    )


def representation_from_str(
    representation: str, language_map: dict[Representation, str]
) -> Representation:
    """
    Get the appropriate representation enum from the representation string.
    Also, apply the right language map so that when printed, the right types are
    used.

    Args:
      representation: The YAML representation string
      language_map: A mapping of a Representation instance to a language
        specific type.
    """
    result: Representation | None = None
    if representation == "float":
        result = Representation.FLOAT
    elif representation == "signed":
        result = Representation.SIGNED
    elif representation == "unsigned":
        result = Representation.UNSIGNED

    assert result is not None
    result.language_map = language_map
    return result


def channel_from_dict(
    channel_id: str,
    definition: dict,
    units: dict[str, Units],
    language_representation_map: dict[Representation, str],
) -> Channel:
    """
    Construct a Channel from a dictionary entry.

    Args:
      channel_id: The ID of the channel in the channel map
      definition: The node condent of ["channels"][channel_id]
      units: A mapping of all the units
      language_representation_map: A mapping of a Representation instance to a
        language specific type.
    """
    return Channel(
        id=channel_id,
        name=definition["name"],
        description=definition["description"],
        representation=representation_from_str(
            definition["representation"],
            language_map=language_representation_map,
        ),
        units=units[definition["units"]],
    )


def trigger_from_dict(trigger_id: str, definition: dict) -> Trigger:
    """
    Construct a Trigger from a dictionary entry.

    Args:
      trigger_id: The ID of the trigger in the trigger map
      definition: The node condent of ["triggers"][trigger_id]
    """
    return Trigger(
        id=trigger_id,
        name=definition["name"],
        description=definition["description"],
    )


def units_from_dict(units_id: str, definition: dict) -> Units:
    """
    Construct a Units from a dictionary entry.

    Args:
      units_id: The ID of the units in the unit map
      definition: The node condent of ["units"][units_id]
    """
    return Units(
        id=units_id,
        name=definition["name"],
        description=definition["description"],
        symbol=definition["symbol"],
    )


class CppHeader:
    """Generator for a C++ header"""

    def __init__(
        self,
        package: Sequence[str],
        attributes: Sequence[Attribute],
        channels: Sequence[Channel],
        triggers: Sequence[Trigger],
        units: Sequence[Units],
    ) -> None:
        """
        Args:
          package: The package name used in the output. In C++ we'll convert
            this to a namespace.
          attributes: A sequence of attributes which will be exposed in the
            'attributes' namespace
          channels: A sequence of channels which will be exposed in the
            'channels' namespace
          triggers: A sequence of triggers which will be exposed in the
            'triggers' namespace
          units: A sequence of units which should be exposed in the 'units'
            namespace
        """
        self._package: str = '::'.join(package)
        self._attributes: Sequence[Attribute] = attributes
        self._channels: Sequence[Channel] = channels
        self._triggers: Sequence[Trigger] = triggers
        self._units: Sequence[Units] = units

    def __str__(self) -> str:
        writer = io.StringIO()
        self._print_header(writer=writer)
        self._print_constants(writer=writer)
        self._print_footer(writer=writer)
        return writer.getvalue()

    def _print_header(self, writer: io.TextIOWrapper) -> None:
        """
        Print the top part of the .h file (pragma, includes, and namespace)

        Args:
          writer: Where to write the text to
        """
        writer.write(
            "/* Auto-generated file, do not edit */\n"
            "#pragma once\n"
            "\n"
            "#include \"pw_sensor/types.h\"\n"
        )
        if self._package:
            writer.write(f"namespace {self._package} {{\n\n")

    def _print_constants(self, writer: io.TextIOWrapper) -> None:
        """
        Print the constants definitions from self._attributes, self._channels,
        and self._trigger

        Args:
            writer: Where to write the text
        """

        self._print_in_namespace(
            namespace="units", printables=self._units, writer=writer
        )
        self._print_in_namespace(
            namespace="attributes", printables=self._attributes, writer=writer
        )
        self._print_in_namespace(
            namespace="channels", printables=self._channels, writer=writer
        )
        self._print_in_namespace(
            namespace="triggers", printables=self._triggers, writer=writer
        )

    @staticmethod
    def _print_in_namespace(
        namespace: str,
        printables: Sequence[Printable],
        writer: io.TextIOWrapper,
    ) -> None:
        """
        Print constants definitions wrapped in a namespace
        Args:
          namespace: The namespace to use
          printables: A sequence of printable objects
          writer: Where to write the text
        """
        writer.write(f"\nnamespace {namespace} {{\n")
        for printable in printables:
            printable.print(writer=writer)
        writer.write(f"\n}}  // namespace {namespace}\n")

    def _print_footer(self, writer: io.TextIOWrapper) -> None:
        """
        Write the bottom part of the .h file (closing namespace)

        Args:
            writer: Where to write the text
        """
        if self._package:
            writer.write(f"\n}}  // namespace {self._package}")


def main() -> None:
    """
    Main entry point, this function will:
    - Get CLI flags
    - Read YAML from stdin
    - Find all attribute, channel, trigger, and unit definitions
    - Print header
    """
    args = get_args()
    spec = yaml.safe_load(sys.stdin)
    all_attributes: set[Attribute] = set()
    all_channels: set[Channel] = set()
    all_triggers: set[Trigger] = set()
    all_units: dict[str, Units] = {}
    language_representation_map: dict[Representation, str] = {}
    if args.language == "cpp":
        language_representation_map[Representation.FLOAT] = "double"
        language_representation_map[Representation.SIGNED] = "int64_t"
        language_representation_map[Representation.UNSIGNED] = "uint64_t"
    for units_id, definition in spec["units"].items():
        units = units_from_dict(units_id=units_id, definition=definition)
        assert units not in all_units.values()
        all_units[units_id] = units
    for attribute_id, definition in spec["attributes"].items():
        attribute = attribute_from_dict(
            attribute_id=attribute_id, definition=definition
        )
        assert not attribute in all_attributes
        all_attributes.add(attribute)
    for channel_id, definition in spec["channels"].items():
        channel = channel_from_dict(
            channel_id=channel_id,
            definition=definition,
            units=all_units,
            language_representation_map=language_representation_map,
        )
        assert not channel in all_channels
        all_channels.add(channel)
    for trigger_id, definition in spec["triggers"].items():
        trigger = trigger_from_dict(
            trigger_id=trigger_id, definition=definition
        )
        assert not trigger in all_triggers
        all_triggers.add(trigger)

    if args.language == "cpp":
        out = CppHeader(
            package=args.package,
            attributes=list(all_attributes),
            channels=list(all_channels),
            triggers=list(all_triggers),
            units=list(all_units.values()),
        )
    else:
        raise ValueError(f"Invalid language selected: '{args.language}'")
    print(out)


def validate_package_arg(value: str) -> str:
    """
    Validate that the package argument is a valid string

    Args:
      value: The package name

    Returns:
      The same value after being validated.
    """
    if value is None or value == "":
        return value
    if not re.match(r"[a-zA-Z_$][\w$]*(\.[a-zA-Z_$][\w$]*)*", value):
        raise argparse.ArgumentError(
            argument=None,
            message=f"Invalid string {value}. Must use alphanumeric values "
            "separated by dots.",
        )
    return value


def get_args() -> Args:
    """
    Get CLI arguments

    Returns:
      Typed arguments class instance
    """
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--package",
        "-pkg",
        default="",
        type=validate_package_arg,
        help="Output package name separated by '.', example: com.google",
    )
    parser.add_argument(
        "--language",
        type=str,
        choices=["cpp"],
        default="cpp",
    )
    args = parser.parse_args()
    return Args(package=args.package.split("."), language=args.language)


if __name__ == "__main__":
    main()
