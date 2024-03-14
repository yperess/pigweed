.. _module-pw_allocator-api:

=============
API reference
=============
.. pigweed-module-subpage::
   :name: pw_allocator
   :tagline: pw_allocator: Flexible, safe, and measurable memory allocation

This module provides the following:

- Generic allocator interfaces that can be injected into routines that need
  dynamic memory. These include :ref:`module-pw_allocator-api-allocator`, as
  well as the :ref:`module-pw_allocator-api-layout` type that is passed to it
  and the :ref:`module-pw_allocator-api-unique_ptr` returned from it.
- Concrete allocator implementations used to provide memory dynamically.
- "Forwarding" allocators, as described by
  :ref:`module-pw_allocator-design-forwarding`.
- Additional allocator utility classes. These are typically used by allocator
  implementers.
- Test utilities for testing allocator implementations. These are typically used
  by allocator implementers.

.. _module-pw_allocator-api-allocator:

------------------------
pw::allocator::Allocator
------------------------
.. doxygenclass:: pw::allocator::Allocator
   :members:

.. _module-pw_allocator-api-layout:

Layout
======
.. doxygenclass:: pw::allocator::Layout
   :members:

.. _module-pw_allocator-api-unique_ptr:

UniquePtr
=========
.. doxygenclass:: pw::allocator::UniquePtr
   :members:

-------------------------
Allocator Implementations
-------------------------
This module provides several concrete allocator implementations of the
:ref:`module-pw_allocator-api-allocator` interface:

.. _module-pw_allocator-api-block_allocator:

BlockAllocator
==============
.. doxygenclass:: pw::allocator::internal::BlockAllocator
   :members:

.. _module-pw_allocator-api-first_fit_block_allocator:

FirstFitBlockAllocator
----------------------
.. doxygenclass:: pw::allocator::FirstFitBlockAllocator
   :members:

.. _module-pw_allocator-api-last_fit_block_allocator:

LastFitBlockAllocator
---------------------
.. doxygenclass:: pw::allocator::LastFitBlockAllocator
   :members:

.. _module-pw_allocator-api-best_fit_block_allocator:

BestFitBlockAllocator
---------------------
.. doxygenclass:: pw::allocator::BestFitBlockAllocator
   :members:

.. _module-pw_allocator-api-worst_fit_block_allocator:

WorstFitBlockAllocator
----------------------
.. doxygenclass:: pw::allocator::WorstFitBlockAllocator
   :members:

.. _module-pw_allocator-api-dual_first_fit_block_allocator:

DualFirstFitBlockAllocator
--------------------------
.. doxygenclass:: pw::allocator::DualFirstFitBlockAllocator
   :members:

.. _module-pw_allocator-api-libc_allocator:

LibCAllocator
=============
.. doxygenclass:: pw::allocator::LibCAllocator
   :members:

.. _module-pw_allocator-api-null_allocator:

NullAllocator
=============
.. doxygenclass:: pw::allocator::NullAllocator
   :members:

.. TODO: b/328076428 - Update FreeListHeap or remove

---------------------
Forwarding Allocators
---------------------
This module provides several "forwarding" allocators, as described in
:ref:`module-pw_allocator-design-forwarding`.

.. _module-pw_allocator-api-fallback_allocator:

FallbackAllocator
=================
.. doxygenclass:: pw::allocator::FallbackAllocator
   :members:

.. _module-pw_allocator-api-synchronized_allocator:

SynchronizedAllocator
=====================
.. doxygenclass:: pw::allocator::SynchronizedAllocator
   :members:

.. _module-pw_allocator-api-tracking_allocator:

TrackingAllocator
=================
.. doxygenclass:: pw::allocator::TrackingAllocatorImpl
   :members:

---------------
Utility Classes
---------------
In addtion to providing allocator implementations themselves, this module
includes some utility classes.

.. _module-pw_allocator-api-block:

Block
=====
.. doxygenclass:: pw::allocator::Block
   :members:

.. _module-pw_allocator-api-metrics_adapter:

Metrics
=======
.. doxygenclass:: pw::allocator::internal::Metrics
   :members:

This class is templated on a ``MetricsType`` struct. See
:ref:`module-pw_allocator-design-metrics` for additional details on how the
struct, this class, and :ref:`module-pw_allocator-api-tracking_allocator`
interact.

This modules also includes two concrete metrics types that can be used as the
template parameter of this class:

- ``AllMetrics``, which enables all metrics.
- ``NoMetrics``, which disables all metrics.

Additionally, projects can define their own metrics structs using the following
macros:

.. doxygendefine:: PW_ALLOCATOR_METRICS_DECLARE
.. doxygendefine:: PW_ALLOCATOR_METRICS_ENABLE

.. _module-pw_allocator-api-size_reporter:

SizeReporter
============
.. doxygenclass:: pw::allocator::SizeReporter
   :members:

WithBuffer
==========
.. doxygenclass:: pw::allocator::WithBuffer
   :members:

------------
Test support
------------
This modules also include test utilities to facilitate writing uint tests and
fuzz tests for both concrete and forwarding allocator implementations.

.. _module-pw_allocator-api-allocator_for_test:

AllocatorForTest
================
.. doxygenclass:: pw::allocator::test::AllocatorForTest
   :members:

.. _module-pw_allocator-api-allocator_test_harness:

AllocatorTestHarness
====================
.. doxygenclass:: pw::allocator::test::AllocatorTestHarness
   :members:

.. _module-pw_allocator-api-fuzzing_support:

FuzzTest support
================
.. doxygenfunction:: pw::allocator::test::ArbitraryAllocatorRequest
.. doxygenfunction:: pw::allocator::test::ArbitraryAllocatorRequests
.. doxygenfunction:: pw::allocator::test::MakeAllocatorRequest
