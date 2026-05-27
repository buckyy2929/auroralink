# ============================================================================
# Cross-compile toolchain: x86_64 Linux host -> aarch64 (Cortex-A53, Kria PS)
#
# Used by:   protocol_translator (M2-C cross-compile of libEGD for Kria PS)
# Consumes:  Debian bookworm aarch64-linux-gnu cross-toolchain (gcc/g++ 12),
#            libssl-dev:arm64 + zlib1g-dev:arm64 via dpkg multiarch,
#            cross-built static libcurl / libnghttp2 / libpsl under
#            /usr/local/aarch64-linux-gnu (symlinked into /usr/local/lib so
#            that libEGD's hard-coded STATIC_DEPS paths in CMakeLists.txt
#            resolve unchanged).
#
# Why the compiler is a wrapper, not the bare aarch64-linux-gnu-gcc:
#   libEGD's CMakeLists.txt unconditionally appends x86-only Intel CET
#   hardening flags (`-fcf-protection=full -mshstk`) to CMAKE_C_FLAGS_RELEASE
#   and CMAKE_CXX_FLAGS_RELEASE *after* this toolchain runs, so simply
#   setting CMAKE_<LANG>_FLAGS_RELEASE here with CACHE FORCE is not enough --
#   the bad flags still land on the command line and aarch64-linux-gnu-gcc
#   will reject `-mshstk` outright.  The wrappers strip those two flags
#   transparently; everything else is forwarded verbatim.  This keeps the
#   upstream GE libEGD source tree untouched (per M2-C design intent).
# ============================================================================

set(CMAKE_SYSTEM_NAME      Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# --- Cross compilers ---------------------------------------------------------
# Inside the libegd-arm64:builder image these wrappers live in /usr/local/bin
# and shell out to /usr/bin/aarch64-linux-gnu-gcc / -g++ (Debian GCC 12).
set(CMAKE_C_COMPILER   /usr/local/bin/aarch64-linux-gnu-gcc-wrapper)
set(CMAKE_CXX_COMPILER /usr/local/bin/aarch64-linux-gnu-g++-wrapper)
set(CMAKE_AR           /usr/bin/aarch64-linux-gnu-ar           CACHE FILEPATH "")
set(CMAKE_RANLIB       /usr/bin/aarch64-linux-gnu-ranlib       CACHE FILEPATH "")
set(CMAKE_STRIP        /usr/bin/aarch64-linux-gnu-strip        CACHE FILEPATH "")
set(CMAKE_LINKER       /usr/bin/aarch64-linux-gnu-ld           CACHE FILEPATH "")
set(CMAKE_OBJCOPY      /usr/bin/aarch64-linux-gnu-objcopy      CACHE FILEPATH "")
set(CMAKE_OBJDUMP      /usr/bin/aarch64-linux-gnu-objdump      CACHE FILEPATH "")

# --- Sysroot / find-root behaviour -------------------------------------------
# Debian places the aarch64 sysroot at /usr/aarch64-linux-gnu and multiarch
# libs/headers under /usr/{lib,include}/aarch64-linux-gnu.  Our own cross-built
# static deps land in /usr/local/aarch64-linux-gnu.
set(CMAKE_FIND_ROOT_PATH
    /usr/aarch64-linux-gnu
    /usr/local/aarch64-linux-gnu
    /usr/lib/aarch64-linux-gnu
)

# Programs (e.g. pkg-config, configure helpers) must come from the build host.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# Libraries, headers and CMake packages must come from the target sysroot only.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config -> arm64 multiarch
set(ENV{PKG_CONFIG_LIBDIR}
    "/usr/lib/aarch64-linux-gnu/pkgconfig:/usr/local/aarch64-linux-gnu/lib/pkgconfig")
set(ENV{PKG_CONFIG_PATH} "")

# --- Cross-built dependency headers ------------------------------------------
# libEGD's CMakeLists.txt does `#include <curl/curl.h>` without ever adding an
# -I for it; the upstream x86 build worked only because Debian's NATIVE gcc
# searches /usr/local/include by default. The aarch64 CROSS gcc does not --
# it has its own sysroot-aware search list. So we explicitly inject the
# header dir for our cross-built libcurl / libnghttp2 / libpsl. _INIT is the
# CMake-blessed hook that toolchain files use to seed CMAKE_<LANG>_FLAGS on
# the very first configure (it survives libEGD's later non-CACHE
# `set(CMAKE_CXX_FLAGS_RELEASE ...)` appends because it lands in CMAKE_<LANG>_FLAGS,
# a different variable).
set(CMAKE_C_FLAGS_INIT   "-I/usr/local/aarch64-linux-gnu/include")
set(CMAKE_CXX_FLAGS_INIT "-I/usr/local/aarch64-linux-gnu/include")

# --- Hardening flags ---------------------------------------------------------
# ARM64 equivalents of what libEGD's Release config was *trying* to do on x86:
#   x86 CET     -fcf-protection=full -mshstk      (control-flow integrity)
#   ARM PAC/BTI -mbranch-protection=standard      (GCC 9+, A53 silently ignores
#                                                  unsupported PAuth keys; safe.)
set(ARM64_HARDENING
    "-D_FORTIFY_SOURCE=2 -fpie -fstack-protector-all -mbranch-protection=standard")
set(ARM64_LINK_HARDENING "-Wl,-z,relro,-z,now")

# CACHE/FORCE so anything that reads these *after* the toolchain runs sees the
# clean baseline.  The wrappers above are the belt-and-braces backstop for the
# case where libEGD's CMakeLists.txt appends the bad x86 flags on top.
set(CMAKE_C_FLAGS_RELEASE
    "-O2 -s ${ARM64_HARDENING}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE
    "-O2 -s ${ARM64_HARDENING}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE
    "${ARM64_LINK_HARDENING}"   CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_RELEASE
    "${ARM64_LINK_HARDENING}"   CACHE STRING "" FORCE)

# Default to Release if the caller didn't pick a build type.
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
