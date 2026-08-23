#===----------------------------------------------------------------------===#
# libcontracts -- the pure-C contracts ABI core runtime.
#
# This is the LLVM build of the same interchangeable library GCC provides:
# identical ABI, SONAME (libcontracts.so.1) and version node
# (LIBCONTRACTS_1.0), implemented independently under the LLVM license.
# It holds the wire-format dispatch, chain walking, the C accessor functions,
# and the P3290 C API / check helpers; it depends only on libc.
#===----------------------------------------------------------------------===#

cmake_minimum_required(VERSION 3.20.0)

set(LLVM_SUBPROJECT_TITLE "libcontracts")

# Standalone-build support (mirrors the other runtimes).  In the in-tree
# runtimes build the framework has already created the project and set up the
# C compiler/target, so we do not call project() again.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR OR LIBCONTRACTS_STANDALONE_BUILD)
  project(libcontracts LANGUAGES C)
  set(LIBCONTRACTS_STANDALONE_BUILD ON)
endif()

include(GNUInstallDirs)

# Place the library next to libc++ so libc++'s DT_NEEDED resolves at run time
# and the existing test harness (which points at the libc++ directory) finds
# it.  Fall back sensibly for a standalone build.
if(DEFINED LLVM_LIBRARY_OUTPUT_INTDIR AND DEFINED LLVM_DEFAULT_TARGET_TRIPLE)
  set(LIBCONTRACTS_OUTPUT_DIR
      "${LLVM_LIBRARY_OUTPUT_INTDIR}/${LLVM_DEFAULT_TARGET_TRIPLE}")
elseif(DEFINED LLVM_LIBRARY_OUTPUT_INTDIR)
  set(LIBCONTRACTS_OUTPUT_DIR "${LLVM_LIBRARY_OUTPUT_INTDIR}")
else()
  set(LIBCONTRACTS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/lib")
endif()

# The pure-C contracts ABI core: dispatch + chain walking + non-noexcept
# entry points, C accessor functions, and the P3290 C API / check helpers.
set(LIBCONTRACTS_SOURCES dispatch.c accessors.c c_api.c)

add_library(contracts_shared SHARED ${LIBCONTRACTS_SOURCES})
set_target_properties(contracts_shared PROPERTIES
  OUTPUT_NAME contracts
  VERSION 1.0
  SOVERSION 1
  LIBRARY_OUTPUT_DIRECTORY "${LIBCONTRACTS_OUTPUT_DIR}"
  RUNTIME_OUTPUT_DIRECTORY "${LIBCONTRACTS_OUTPUT_DIR}")
target_link_options(contracts_shared PRIVATE
  "LINKER:--version-script,${CMAKE_CURRENT_SOURCE_DIR}/libcontracts.map")

add_library(contracts_static STATIC ${LIBCONTRACTS_SOURCES})
set_target_properties(contracts_static PROPERTIES
  OUTPUT_NAME contracts
  ARCHIVE_OUTPUT_DIRECTORY "${LIBCONTRACTS_OUTPUT_DIR}")

# Install next to libc++ (mirroring the build-output layout above): with the
# per-target runtime directory the toolchain's stdlib search path -- the only
# -L the clang driver adds for -lcontracts -- is <prefix>/lib/<triple>, NOT the
# bare <prefix>/lib.  Installing to the bare libdir leaves -lcontracts unfound
# at link time even though libc++ (same dir in the build) resolves fine.
#
# Use lib${LLVM_LIBDIR_SUFFIX}, NOT ${CMAKE_INSTALL_LIBDIR}, because only the
# former is what libc++ itself uses:
#   libcxx/CMakeLists.txt:483
#     set(LIBCXX_INSTALL_LIBRARY_DIR lib${LLVM_LIBDIR_SUFFIX}/${LIBCXX_TARGET_SUBDIR} ...)
# CMAKE_INSTALL_LIBDIR comes from GNUInstallDirs and is DISTRO-DEPENDENT: it is
# "lib" on Debian/Ubuntu but "lib64" on RHEL-family systems.  Building this
# toolchain in a rhel8 Bloomberg Space therefore put libcontracts in
# <prefix>/lib64/<triple> while libc++ went to <prefix>/lib/<triple>, and
# -lcontracts stopped resolving at link time -- every Clang contracts example
# failed with "/usr/bin/ld: cannot find -lcontracts".  The two must be derived
# the same way or they silently diverge on any non-Debian build host.
if(LLVM_ENABLE_PER_TARGET_RUNTIME_DIR AND DEFINED LLVM_DEFAULT_TARGET_TRIPLE)
  set(LIBCONTRACTS_INSTALL_LIBDIR
      "lib${LLVM_LIBDIR_SUFFIX}/${LLVM_DEFAULT_TARGET_TRIPLE}")
else()
  set(LIBCONTRACTS_INSTALL_LIBDIR "lib${LLVM_LIBDIR_SUFFIX}")
endif()

# Name an install component so consumers can stage libcontracts on its own --
# in particular libc++'s test-suite-install prefix, which installs per
# component and would otherwise ship a libc++ whose libcontracts dependency
# cannot be resolved at run time.
install(TARGETS contracts_shared contracts_static
  LIBRARY DESTINATION ${LIBCONTRACTS_INSTALL_LIBDIR} COMPONENT contracts
  ARCHIVE DESTINATION ${LIBCONTRACTS_INSTALL_LIBDIR} COMPONENT contracts)
