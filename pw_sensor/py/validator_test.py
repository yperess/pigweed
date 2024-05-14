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
"""Unit tests for the sensor metadata validator"""

from pathlib import Path
import unittest
import tempfile
import yaml
from pw_sensor.validator import Validator


class ValidatorTest(unittest.TestCase):
    """Tests the Validator class."""

    maxDiff = None

    def test_missing_compatible(self) -> None:
        """Check that missing 'compatible' key throws exception"""
        self._check_with_exception(
            metadata={},
            exception_string="ERROR: Malformed sensor metadata YAML:\n{}",
            cause_substrings=["'compatible' is a required property"],
        )

    def test_invalid_compatible_type(self) -> None:
        """Check that incorrect type of 'compatible' throws exception"""
        self._check_with_exception(
            metadata={"compatible": {}},
            exception_string=(
                "ERROR: Malformed sensor metadata YAML:\ncompatible: {}"
            ),
            cause_substrings=[
                "'org' is a required property",
            ],
        )

        self._check_with_exception(
            metadata={"compatible": []},
            exception_string=(
                "ERROR: Malformed sensor metadata YAML:\ncompatible: []"
            ),
            cause_substrings=["[] is not of type 'object'"],
        )

        self._check_with_exception(
            metadata={"compatible": 1},
            exception_string=(
                "ERROR: Malformed sensor metadata YAML:\ncompatible: 1"
            ),
            cause_substrings=["1 is not of type 'object'"],
        )

        self._check_with_exception(
            metadata={"compatible": ""},
            exception_string=(
                "ERROR: Malformed sensor metadata YAML:\ncompatible: ''"
            ),
            cause_substrings=[" is not of type 'object'"],
        )

    def test_empty_dependency_list(self) -> None:
        """
        Check that an empty or missing 'deps' resolves to one with an empty
        'deps' list
        """
        expected = {
            "sensors": {
                "google,foo": {
                    "compatible": {"org": "google", "part": "foo"},
                    "channels": {},
                    "attributes": [],
                    "triggers": [],
                },
            },
            "channels": {},
            "attributes": {},
            "triggers": {},
            "units": {},
        }
        metadata = {
            "compatible": {"org": "google", "part": "foo"},
            "deps": [],
        }
        result = Validator().validate(metadata=metadata)
        self.assertEqual(result, expected)

        metadata = {"compatible": {"org": "google", "part": "foo"}}
        result = Validator().validate(metadata=metadata)
        self.assertEqual(result, expected)

    def test_invalid_dependency_file(self) -> None:
        """
        Check that if an invalid dependency file is listed, we throw an error.
        We know this will not be a valid file, because we have no files in the
        include path so we have nowhere to look for the file.
        """
        self._check_with_exception(
            metadata={
                "compatible": {"org": "google", "part": "foo"},
                "deps": ["test.yaml"],
            },
            exception_string="Failed to find test.yaml using search paths:",
            cause_substrings=[],
            exception_type=FileNotFoundError,
        )

    def test_invalid_channel_name_raises_exception(self) -> None:
        """
        Check that if given a channel name that's not defined, we raise an Error
        """
        self._check_with_exception(
            metadata={
                "compatible": {"org": "google", "part": "foo"},
                "channels": {"bar": []},
            },
            exception_string="Failed to find a definition for 'bar', did"
            " you forget a dependency?",
            cause_substrings=[],
        )

    def test_channel_info_from_deps(self) -> None:
        """
        End to end test resolving a dependency file and setting the right
        default attribute values.
        """
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".yaml", encoding="utf-8", delete=False
        ) as dep:
            dep_filename = Path(dep.name)
            dep.write(
                yaml.safe_dump(
                    {
                        "units": {
                            "rate": {
                                "symbol": "Hz",
                            },
                            "sandwiches": {
                                "symbol": "sandwiches",
                            },
                            "squeaks": {"symbol": "squeaks"},
                            "items": {
                                "symbol": "items",
                            },
                        },
                        "attributes": {
                            "sample_rate": {},
                        },
                        "channels": {
                            "bar": {
                                "units": "sandwiches",
                                "representation": "unsigned",
                            },
                            "soap": {
                                "name": "The soap",
                                "description": (
                                    "Measurement of how clean something is"
                                ),
                                "units": "squeaks",
                            },
                            "laundry": {
                                "description": "Clean clothes count",
                                "units": "items",
                                "sub-channels": {
                                    "shirts": {
                                        "description": "Clean shirt count",
                                    },
                                    "pants": {
                                        "description": "Clean pants count",
                                    },
                                },
                            },
                        },
                        "triggers": {
                            "data_ready": {
                                "description": "notify when new data is ready",
                            },
                        },
                    },
                )
            )

        metadata = Validator(include_paths=[dep_filename.parent]).validate(
            metadata={
                "compatible": {"org": "google", "part": "foo"},
                "deps": [dep_filename.name],
                "attributes": [
                    {
                        "attribute": "sample_rate",
                        "channel": "laundry",
                        "units": "rate",
                    },
                ],
                "channels": {
                    "bar": [],
                    "soap": [],
                    "laundry_shirts": [],
                    "laundry_pants": [],
                    "laundry": [
                        {"name": "kids' laundry"},
                        {"name": "adults' laundry"},
                    ],
                },
                "triggers": [
                    "data_ready",
                ],
            },
        )

        # Check attributes
        self.assertEqual(
            metadata,
            {
                "attributes": {
                    "sample_rate": {
                        "name": "sample_rate",
                        "description": "",
                    },
                },
                "channels": {
                    "bar": {
                        "name": "bar",
                        "description": "",
                        "representation": "unsigned",
                        "units": "sandwiches",
                    },
                    "soap": {
                        "name": "The soap",
                        "description": "Measurement of how clean something is",
                        "representation": "float",
                        "units": "squeaks",
                    },
                    "laundry": {
                        "name": "laundry",
                        "description": "Clean clothes count",
                        "representation": "float",
                        "units": "items",
                    },
                    "laundry_shirts": {
                        "name": "laundry_shirts",
                        "description": "Clean shirt count",
                        "representation": "float",
                        "units": "items",
                    },
                    "laundry_pants": {
                        "name": "laundry_pants",
                        "description": "Clean pants count",
                        "representation": "float",
                        "units": "items",
                    },
                },
                "triggers": {
                    "data_ready": {
                        "name": "data_ready",
                        "description": "notify when new data is ready",
                    },
                },
                "units": {
                    "rate": {"name": "Hz", "symbol": "Hz"},
                    "sandwiches": {
                        "name": "sandwiches",
                        "symbol": "sandwiches",
                    },
                    "squeaks": {"name": "squeaks", "symbol": "squeaks"},
                    "items": {"name": "items", "symbol": "items"},
                },
                "sensors": {
                    "google,foo": {
                        "compatible": {
                            "org": "google",
                            "part": "foo",
                        },
                        "attributes": [
                            {
                                "attribute": "sample_rate",
                                "channel": "laundry",
                                "units": "rate",
                                "representation": "float",
                            },
                        ],
                        "channels": {
                            "bar": [
                                {
                                    "name": "bar",
                                    "description": "",
                                },
                            ],
                            "soap": [
                                {
                                    "name": "The soap",
                                    "description": (
                                        "Measurement of how clean something is"
                                    ),
                                },
                            ],
                            "laundry": [
                                {
                                    "name": "kids' laundry",
                                    "description": "Clean clothes count",
                                },
                                {
                                    "name": "adults' laundry",
                                    "description": "Clean clothes count",
                                },
                            ],
                            "laundry_shirts": [
                                {
                                    "name": "laundry_shirts",
                                    "description": "Clean shirt count",
                                },
                            ],
                            "laundry_pants": [
                                {
                                    "name": "laundry_pants",
                                    "description": "Clean pants count",
                                },
                            ],
                        },
                        "triggers": ["data_ready"],
                    },
                },
            },
        )

    def _check_with_exception(
        self,
        metadata: dict,
        exception_string: str,
        cause_substrings: list[str],
        exception_type: type[BaseException] = RuntimeError,
    ) -> None:
        with self.assertRaises(exception_type) as context:
            Validator().validate(metadata=metadata)

        self.assertEqual(str(context.exception).rstrip(), exception_string)
        for cause_substring in cause_substrings:
            self.assertTrue(
                cause_substring in str(context.exception.__cause__),
                f"Actual cause: {str(context.exception.__cause__)}",
            )


if __name__ == "__main__":
    unittest.main()
