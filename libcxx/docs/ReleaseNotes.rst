=========================================
Libc++ 17.0.0 (In-Progress) Release Notes
=========================================

.. contents::
   :local:
   :depth: 2

Written by the `Libc++ Team <https://libcxx.llvm.org>`_

.. warning::

   These are in-progress notes for the upcoming libc++ 17 release.
   Release notes for previous releases can be found on
   `the Download Page <https://releases.llvm.org/download.html>`_.

Introduction
============

This document contains the release notes for the libc++ C++ Standard Library,
part of the LLVM Compiler Infrastructure, release 17.0.0. Here we describe the
status of libc++ in some detail, including major improvements from the previous
release and new feature work. For the general LLVM release notes, see `the LLVM
documentation <https://llvm.org/docs/ReleaseNotes.html>`_. All LLVM releases may
be downloaded from the `LLVM releases web site <https://llvm.org/releases/>`_.

For more information about libc++, please see the `Libc++ Web Site
<https://libcxx.llvm.org>`_ or the `LLVM Web Site <https://llvm.org>`_.

Note that if you are reading this file from a Git checkout or the
main Libc++ web page, this document applies to the *next* release, not
the current one. To see the release notes for a specific release, please
see the `releases page <https://llvm.org/releases/>`_.

What's New in Libc++ 17.0.0?
============================

There is an experimental implementation of the C++23 ``std`` module. See
:ref:`ModulesInLibcxx` for more information.

Implemented Papers
------------------
- P2520R0 - ``move_iterator<T*>`` should be a random access iterator
- P1328R1 - ``constexpr type_info::operator==()``
- P1413R3 - Formatting ``thread::id`` (the ``stacktrace`` is not done yet)
- P2675R1 - ``format``'s width estimation is too approximate and not forward compatible
- P2505R5 - Monadic operations for ``std::expected``
- P2711R1 - Making Multi-Param Constructors Of views explicit (``join_with_view`` is not done yet)
- P2572R1 - ``std::format`` fill character allowances
- P2510R3 - Formatting pointers

Improvements and New Features
-----------------------------
- ``std::equal`` and ``std::ranges::equal`` are now forwarding to ``std::memcmp`` for integral types and pointers,
  which can lead up to 40x performance improvements.

- ``std::string_view`` now provides iterators that check for out-of-bounds accesses when the safe
  libc++ mode is enabled.

- The performance of ``dynamic_cast`` on its hot paths is greatly improved and is as efficient as the
  ``libsupc++`` implementation. Note that the performance improvements are shipped in ``libcxxabi``.

- `D122780 <https://reviews.llvm.org/D122780>`_ Improved the performance of ``std::sort`` and ``std::ranges::sort``
  by up to 50% for arithmetic types and by approximately 10% for other types.

- The ``<format>`` header is no longer considered experimental. Some
  ``std::formatter`` specializations are not yet available since the class used
  in the specialization has not been implemented in libc++. This prevents the
  feature-test macro to be set.

- Platforms that don't have support for a filesystem can now still take advantage of some parts of ``<filesystem>``.
  Anything that does not rely on having an actual filesystem available will now work, such as ``std::filesystem::path``,
  ``std::filesystem::perms`` and similar classes.

- The library now provides a hardened mode under which common cases of library undefined behavior will be turned into
  a reliable program termination. Vendors can configure whether the hardened mode is enabled by default with the
  ``LIBCXX_HARDENING_MODE`` variable at CMake configuration time. Users can control whether the hardened mode is
  enabled on a per translation unit basis using the ``-D_LIBCPP_ENABLE_HARDENED_MODE=1`` macro. See
  ``libcxx/docs/HardenedMode.rst`` for more details.

- The library now provides a debug mode which is a superset of the hardened mode, additionally enabling more expensive
  checks that are not suitable to be used in production. This replaces the legacy debug mode that was removed in this
  release. Unlike the legacy debug mode, this doesn't affect the ABI and doesn't require locking. Vendors can configure
  whether the debug mode is enabled by default with the ``LIBCXX_HARDENING_MODE`` variable at CMake configuration time.
  Users can control whether the debug mode is enabled on a per translation unit basis using the
  ``-D_LIBCPP_ENABLE_DEBUG_MODE=1`` macro. See ``libcxx/docs/HardenedMode.rst`` for more details.

Deprecations and Removals
-------------------------

- The "safe" mode is replaced by the hardened mode in this release. The ``LIBCXX_ENABLE_ASSERTIONS`` CMake variable is
  deprecated and setting it will trigger an error; use ``LIBCXX_HARDENING_MODE`` instead. Similarly, the
  ``_LIBCPP_ENABLE_ASSERTIONS`` macro is deprecated and setting it to ``1`` now enables the hardened mode. See
  ``libcxx/docs/HardenedMode.rst`` for more details.

- The legacy debug mode has been removed in this release. Setting the macro ``_LIBCPP_ENABLE_DEBUG_MODE`` to ``1`` now
  enables the new debug mode which is part of hardening (see the "Improvements and New Features" section above). The
  ``LIBCXX_ENABLE_DEBUG_MODE`` CMake variable has been removed. For additional context, refer to the `Discourse post
  <https://discourse.llvm.org/t/rfc-removing-the-legacy-debug-mode-from-libc/71026>`_.

- The ``<experimental/coroutine>`` header has been removed in this release. The ``<coroutine>`` header
  has been shipping since LLVM 14, so the Coroutines TS implementation is being removed per our policy
  for removing TSes.

- Several incidental transitive includes have been removed from libc++. Those
  includes are removed based on the language version used. Incidental transitive
  inclusions of the following headers have been removed:

  - C++23: ``atomic``, ``bit``, ``cstdint``, ``cstdlib``, ``cstring``, ``initializer_list``, ``limits``, ``new``,
           ``stdexcept``, ``system_error``, ``type_traits``, ``typeinfo``

- ``<algorithm>`` no longer includes ``<chrono>`` in any C++ version (it was previously included in C++17 and earlier).

- ``<string>`` no longer includes ``<vector>`` in any C++ version (it was previously included in C++20 and earlier).

- ``<string>``, ``<string_view>``, and ``<mutex>`` no longer include ``<functional>``
  in any C++ version (it was previously included in C++20 and earlier).

- ``<atomic>``, ``<barrier>``, ``<latch>``, ``<numeric>``, ``<semaphore>`` and ``<shared_mutex>`` no longer include ``<iosfwd>``
  (it was previously included in all Standard versions).

- The headers ``<experimental/algorithm>`` and ``<experimental/functional>`` have been removed, since all the contents
  have been implemented in namespace ``std`` for at least two releases.

- The formatter specialization ``template<size_t N> struct formatter<const charT[N], charT>``
  has been removed. Since libc++'s format library was marked experimental there
  is no backwards compatibility option. This specialization has been removed
  from the Standard since it was never used, the proper specialization to use
  instead is ``template<size_t N> struct formatter<charT[N], charT>``.

- Libc++ used to provide some C++11 tag type global variables in C++03 as an extension, which are removed in
  this release. Those variables were ``std::allocator_arg``, ``std::defer_lock``, ``std::try_to_lock``,
  ``std::adopt_lock``, and ``std::piecewise_construct``. Note that the types associated to those variables are
  still provided in C++03 as an extension (e.g. ``std::piecewise_construct_t``). Providing those variables in
  C++03 mode made it impossible to define them properly -- C++11 mandated that they be ``constexpr`` variables,
  which is impossible in C++03 mode. Furthermore, C++17 mandated that they be ``inline constexpr`` variables,
  which led to ODR violations when mixed with the C++03 definition. Cleaning this up is required for libc++ to
  make progress on support for C++20 modules.

- The ``_LIBCPP_ABI_OLD_LOGNORMAL_DISTRIBUTION`` macro has been removed.

- The classes ``strstreambuf`` , ``istrstream``, ``ostrstream``, and ``strstream`` have been deprecated.
  They have been deprecated in the Standard since C++98, but were never marked as deprecated in libc++.

- LWG3631 ``basic_format_arg(T&&) should use remove_cvref_t<T> throughout`` removed
  support for ``volatile`` qualified formatters.

Upcoming Deprecations and Removals
----------------------------------

LLVM 18
~~~~~~~

- The base template for ``std::char_traits`` has been marked as deprecated and
  will be removed in LLVM 18. If you are using ``std::char_traits`` with types
  other than ``char``, ``wchar_t``, ``char8_t``, ``char16_t``, ``char32_t`` or
  a custom character type for which you specialized ``std::char_traits``, your code
  will stop working when we remove the base template. The Standard does not
  mandate that a base template is provided, and such a base template is bound
  to be incorrect for some types, which could currently cause unexpected
  behavior while going undetected.

- The ``_LIBCPP_AVAILABILITY_CUSTOM_VERBOSE_ABORT_PROVIDED`` macro will not be honored anymore in LLVM 18.
  Please see the updated documentation about the safe libc++ mode and in particular the ``_LIBCPP_VERBOSE_ABORT``
  macro for details.

- The headers ``<experimental/deque>``, ``<experimental/forward_list>``, ``<experimental/list>``,
  ``<experimental/map>``, ``<experimental/memory_resource>``, ``<experimental/regex>``, ``<experimental/set>``,
  ``<experimental/string>``, ``<experimental/unordered_map>``, ``<experimental/unordered_set>``,
  and ``<experimental/vector>`` will be removed in LLVM 18, as all their contents will have been implemented in
  namespace ``std`` for at least two releases.

API Changes
-----------

ABI Affecting Changes
---------------------

- Symbols for ``std::allocator_arg``, ``std::defer_lock``, ``std::try_to_lock``, ``std::adopt_lock``, and
  ``std::piecewise_construct`` have been removed from the built library. Under most circumstances, user code
  should not have been relying on those symbols anyway since those are empty classes and the compiler does
  not generate an undefined reference unless the address of the object is taken. However, this is an ABI break
  if the address of one of these objects has been taken in code compiled as C++03, since in those cases the
  objects were marked as defined in the shared library. In other Standard modes, this should never be a problem
  since those objects were defined in the headers as ``constexpr``.

Build System Changes
--------------------

- Building libc++ and libc++abi for Apple platforms now requires targeting macOS 10.13 and later.
  This is relevant for vendors building the libc++ shared library and for folks statically linking
  libc++ into an application that has back-deployment requirements on Apple platforms.

- ``LIBCXX_ENABLE_FILESYSTEM`` now represents whether a filesystem is supported on the platform instead
  of representing merely whether ``<filesystem>`` should be provided. This means that vendors building
  with ``LIBCXX_ENABLE_FILESYSTEM=OFF`` will now also get ``<fstream>`` excluded from their configuration
  of the library.

- ``LIBCXX_ENABLE_FSTREAM`` is not supported anymore, please use ``LIBCXX_ENABLE_FILESYSTEM=OFF`` if your
  platform does not have support for a filesystem.

- The lit test parameter ``enable_modules`` changed from a Boolean to an enum. The changes are

  - ``False`` became ``none``. This option does not test with modules enabled.
  - ``True`` became ``clang``. This option tests using Clang modules.
  - ``std`` is a new optional and tests with the experimental C++23 ``std`` module.
