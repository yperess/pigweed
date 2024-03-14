.. _module-pw_build_android:

================
pw_build_android
================
.. pigweed-module::
   :name: pw_build_android
   :tagline: Utilities for using Pigweed in Android platform
   :status: unstable
   :languages: C++20

``pw_build_android`` provides simple utilities and guidelines for building with
Soong.

----------
Quickstart
----------
Use the ``pw_android_common_backends`` ``cc_defaults`` in the Pigweed module
library or binary rule to use a preselected set of backends common to most
Android platform projects.

.. code-block:: javascript

   cc_binary {
       name: "my_app",
       host_supported: true,
       vendor: true,
       defaults: [
           "pw_android_common_backends",
       ],
       srcs: [
           "main.cc",
       ],
   }


----------------------------
Basic blueprint files format
----------------------------
All Pigweed Soong blueprint files must be named ``Android.bp``, and include the
folowing copyright header with the year adjusted and package.

.. code-block:: javascript

   // Copyright 2024 The Pigweed Authors
   //
   // Licensed under the Apache License, Version 2.0 (the "License"); you may not
   // use this file except in compliance with the License. You may obtain a copy of
   // the License at
   //
   //     https://www.apache.org/licenses/LICENSE-2.0
   //
   // Unless required by applicable law or agreed to in writing, software
   // distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
   // WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
   // License for the specific language governing permissions and limitations under
   // the License.

   package {
       default_applicable_licenses: ["external_pigweed_license"],
   }

The ``bpfmt`` tool in Android can be used to format a blueprint file, e.g.
``bpfmt -w Android.bp``.

-----------------------
Common Android backends
-----------------------
Soong doesn't provide a simple way to select custom Pigweed backends, as is
found in other build systems. Fortunately, most Android platform binaries will
likely use the same common set of backends. Thus, Pigweed provides a
``cc_defaults`` rule named ``pw_android_common_backends`` with a selected set of
common backends for Android platform binaries. This
``pw_android_common_backends`` rule will be used in all Pigweed modules and
backends defined using Soong.

.. _module-pw_build_android-module-libraries:

----------------
Module libraries
----------------
Module libraries are defined as ``cc_library_static`` rules and include the
common Android backends via the ``pw_android_common_backends`` defaults.

.. note::

   Every dependency has to be added as ``whole_static_libs`` to avoid dropping
   symbols on transitive dependencies.

.. code-block:: javascript

   cc_library_static {
       name: "pw_<MODULE_NAME>",
       cpp_std: "c++20",
       export_include_dirs: ["public"],
       vendor_available: true,
       host_supported: true,
       defaults: [
           "pw_android_common_backends",
       ],
       header_libs: [
           // Header library list for all the libraries in #include directives.
       ],
       export_header_lib_headers: [
           // Header library list for all the libraries in #include directives
           // in public header files only.
           // These entries must also be present in header_libs.
       ],
       whole_static_libs: [
           // Static library list for all static library dependencies, listed as
           // whole libraries to avoid dropping symbols in transitive
           // dependencies.
       ],
       export_static_lib_headers: [
           // Static library list for static libraries in #include directives in
           // public header files only.
           // These entries must also be present in whole_static_libs.
       ],
       srcs: [
           // List of source (.cc) files.
       ],
   }

Module libraries with custom backends
-------------------------------------
If a Pigweed module needs to be used with a backend different than the common
Android backend, it should be defined as a ``cc_defaults`` rule following the
``pw_<MODULE_NAME>_no_backends`` name format, with the source files listed in a
``filegroup`` following the ``pw_<MODULE_NAME>_files`` name format. A
``cc_static_library`` with the common Android backend should still be provided,
which uses the mentioned ``cc_defaults``.

.. note::

   ``filegroup`` captures the absolute paths of the listed source files, so they
   can be addressed properly when the ``cc_defaults`` rule is used.

.. code-block:: javascript

   filegroup {
       name: "pw_<MODULE_NAME>_files",
       srcs: [
           // List of source (.cc) files.
       ],
   }

   cc_defaults {
       name: "pw_<MODULE_NAME>_no_backends",
       cpp_std: "c++20",
       export_include_dirs: ["public"],

       header_libs: [
           // Header library list for all the libraries in #include directives.
       ],
       export_header_lib_headers: [
           // Header library list for all the libraries in #include directives
           // in public header files only.
           // These entries must also be present in header_libs.
       ],
       whole_static_libs: [
           // Static library list for all static library dependencies, listed as
           // whole libraries to avoid dropping symbols in transitive
           // dependencies.
       ],
       export_static_lib_headers: [
           // Static library list for static libraries in #include directives in
           // public header files only.
           // These entries must also be present in whole_static_libs.
       ],
       srcs: [
           "pw_<MODULE_NAME>_files",
       ],
   }

   cc_defaults {
       name: "pw_<MODULE_NAME>",
       cpp_std: "c++20",
       export_include_dirs: ["public"],
       defaults: [
           "pw_android_common_backends",
           "pw_<MODULE_NAME>_no_backends",
       ],
       vendor_available: true,
       host_supported: true,
   }


-------
Facades
-------
All facades must be defined as ``cc_library_headers`` if they don’t have
``srcs`` list. The facade names follow the ``pw_<MODULE_NAME>.<FACADE_NAME>``.
In the case there is only one facade in the module or ``<MODULE_NAME>`` is
the same as ``<FACADE_NAME>`` follow ``pw_<MODULE_NAME>``, e.g. ``pw_log``.

.. note::
   Facade names should not be suffixed with ``_headers``.

.. code-block:: javascript

   cc_library_headers {
       name: "pw_<MODULE_NAME>.<FACADE_NAME>",
       cpp_std: "c++20",
       vendor_available: true,
       host_supported: true,
       export_include_dirs: ["public"],
   }

If a facade requires a ``srcs`` list, it must be defined as a ``cc_defaults``
rule instead, with the source files listed in a ``filegroup`` following the
``pw_<MODULE_NAME>.<FACADE_NAME>_files`` name format or
``pw_<MODULE_NAME>_files`` if applicable.

.. note::

   ``filegroup`` captures the absolute paths of the listed source files, so they
   can be addressed properly when the ``cc_defaults`` rule is used.

.. note::

   Facades cannot be defined as ``cc_static_library`` because it wouldn’t be
   possible to build the facade without a backend.

.. code-block:: javascript

   filegroup {
       name: "pw_<MODULE_NAME>.<FACADE_NAME>_files",
       srcs: [
           // List of source (.cc) files.
       ],
   }

   cc_defaults {
       name: "pw_<MODULE_NAME>.<FACADE_NAME>",
       cpp_std: "c++20",
       export_include_dirs: ["public"],
       srcs: [
           "pw_<MODULE_NAME>.<FACADE_NAME>_files",
       ],
   }

To assign a backend to a facade defined as ``cc_defaults`` the ``cc_defaults``
rule can be placed in the ``defaults`` list of a ``cc_static_library`` rule or
binary rule that lists the facade's backend as a dependency.

.. code-block:: javascript

   cc_static_library {
       name: "user_of_pw_<MODULE_NAME>.<FACADE_NAME>",
       cpp_std: "c++20",
       vendor_available: true,
       host_supported: true,
       defaults: [
           "pw_<MODULE_NAME>.<FACADE_NAME>",
       ],
       static_libs: [
           "backend_of_pw_<MODULE_NAME>.<FACADE_NAME>",
       ],
   }

Alternatively, the ``cc_defaults`` rule can be placed in the ``defaults`` list
of another ``cc_defaults`` rule where the latter rule may or may not list the
facade's backend. ``cc_defaults`` rules can be inherited many times. Facades
can be used as long as the backends are assigned in ``cc_static_library`` or
binary rules using the final ``cc_defaults`` as explained above.

--------
Backends
--------
Backends are defined the same way as
:ref:`module-pw_build_android-module-libraries`. They must follow the
``pw_<MODULE_NAME>.<FACADE_NAME>_<BACKEND_NAME>`` name format or
``pw_<MODULE_NAME>_<BACKEND_NAME>`` if applicable.
