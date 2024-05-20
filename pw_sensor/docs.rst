.. _module-pw_sensor:

=========
pw_sensor
=========
This is the main documentation file for pw_sensor. It is under construction.

.. toctree::
   :maxdepth: 1

   py/docs

Defining types
==============
Pigweed provides a meta-description layer for sensors in order to optimize the
build time validation and memory allocation. It's possible to read about these
in :ref:`module-pw_sensor-py`. Once we define our units, measurements,
attributes, and triggers it's possible to create a sensor which leverages these.

Here's an example sensor definition YAML file for a custom sensor made by a
company called "MyOrg" with a part ID "MyPaRt12345". This sensor supports
reading acceleration and it's internal die temperature. We can also configure
the sample rate for the acceleration readings.

.. code-block:: yaml

   deps:
      - "third_party/pigweed/pw_sensor/attributes.yaml"
      - "third_party/pigweed/pw_sensor/channels.yaml"
      - "third_party/pigweed/pw_sensor/units.yaml"
   compatible:
      org: "MyOrg"
      part: "MyPaRt12345"
   channels:
      acceleration: []
      die_temperature: []
   attributes:
      -  attribute: "sample_rate"
         channel: "acceleration"
         units: "frequency"
         representation: "unsigned"

Great, so we have our sensor spec in a YAML file, but we want to use it in code.

.. tab-set::

   .. tab-item:: Bazel

      Under construction

   .. tab-item:: GN

      Under construction

   .. tab-item:: CMake

      Compiling one or more sensor YAML files into a header is done by a call to
      the ``pw_sensor_library`` function. It looks like:

      .. code-block::

         include($ENV{PW_ROOT}/pw_sensor/sensor.cmake)

         # Generate an interface library called my_sensor_lib which exposes a
         # header file that can be included as
         # "my_app/generated/sensor_constants.h".
         pw_sensor_library(my_sensor_lib
            OUT_HEADER
               my_app/generated/sensor_constants.h
            DEPENDS
               $ENV{PW_ROOT}/attributes.yaml
               $ENV{PW_ROOT}/channels.yaml
               $ENV{PW_ROOT}/triggers.yaml
            INCLUDES
               $ENV{PW_ROOT}
            SOURCES
               my_sensor0.yaml
               my_sensor1.yaml
         )

--------------
Under the hood
--------------

In order to communicate with Pigweed's sensor stack, there are a few type
definitions that are used:

* Unit types - created with ``PW_SENSOR_UNIT_TYPE``. These can be thought of as
  defining things like "meters", "meters per second square",
  "radians per second", etc.
* Measurement types - created with ``PW_SENSOR_MEASUREMENT_TYPE``. These are
  different things you can measure with a given unit. Examples: "height",
  "width", and "length" would all use "meters" as a unit but are different
  measurement types.
* Attribute types - created with ``PW_SENSOR_ATTRIBUTE_TYPE``. These are
  configurable aspects of the sensor. They are things like sample rates, tigger
  thresholds, etc. Attributes are unitless until they are paired with the
  measurement type that they modify. As an example "range" attribute for
  acceleration measurements would be in "m/s^2", while a "range" attribute for
  rotational velocity would be in "rad/s".
* Attribute instances - created with ``PW_SENSOR_ATTRIBUTE_INSTANCE``. These
  lump together an attribute with the measurement it applies to along with a
  unit to use. Example: Attribute("sample rate") + Measurement("acceleration") +
  Unit("frequency").
* Trigger types - created with ``PW_SENSOR_TRIGGER_TYPE``. These are events that
  affect the streaming API. These can be events like "fifo full", "tap",
  "double tap"

Developers don't need to actually touch these, as they're automatically called
from the generated sensor library above. The important thing from the input YAML
file is that our final generated header will include the following types:
``attributes::kSampleRate``, ``channels::kAcceleration``,
``channels::kDieTemperature``, and ``units::kFrequency``. All of these are used
by our sensor.

A later change will also introduce the ``PW_SENSOR_ATTRIBUTE_INSTANCE``.
