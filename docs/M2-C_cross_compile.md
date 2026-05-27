# M2-C — Cross-Compile libEGD for Cortex-A53 (Kria PS)

**Status:** Build pipeline implemented and verified on x86_64 build host. `libEGD.so`, `libEGD.a` and the example binaries (`JsonPublishC`, `JsonSubscribeC`, `ListAvailablePlugins`, `RawBytes*`) all confirmed as `ELF 64-bit LSB ... ARM aarch64 ... interpreter /lib/ld-linux-aarch64.so.1` via `file(1)`. Smoke test on A53 hardware pending Kria board access.  
**Sources:** `libEGD-master/CMakeLists.txt`, `libEGD-master/Makefile`, `libEGD-master/hack/docker/Dockerfile.builder`

---

## 1. Build System Analysis

The existing libEGD build system is Docker + CMake. Key facts relevant to ARM64:

| Finding | Impact |
|---|---|
| `ALL_ARCH := amd64` hardcoded in Makefile | ARM64 not supported out of the box |
| Deps (libcurl, libpsl, libnghttp2) built from source inside Docker | Must cross-compile deps too — can't use apt packages for static `.a` files |
| Release flags include `-fcf-protection=full -mshstk` | **x86-only (Intel CET).** `aarch64-linux-gnu-gcc` rejects `-mshstk` outright. Toolchain alone cannot fix it because libEGD's `CMakeLists.txt` *appends* them after the toolchain runs — handled by per-language compiler wrappers, see §4. |
| Static dep paths hardcoded: `/usr/local/lib/libcurl.a` etc. | Solvable by symlinking cross-built deps to that path inside Docker |
| Produces both `libEGD.so` and `libEGD.a` + headers | Both artifacts will be available for the bridge application |

---

## 2. Approach — Docker Cross-Compile

**Strategy:** Extend the existing Docker build pattern with a new `Dockerfile.builder.arm64` that installs the `aarch64-linux-gnu` cross-compiler and cross-compiles all dependencies before building libEGD via a CMake toolchain file.

This avoids modifying GE's libEGD source and integrates cleanly with the existing project structure.

```
Dev host (x86)
  └─ Docker (debian:bookworm-slim + aarch64-linux-gnu toolchain)
       ├─ cross-compile: libpsl, libnghttp2, libcurl (static, ARM64)
       └─ cmake --toolchain aarch64-linux-gnu.cmake
            └─ libEGD.so + libEGD.a (ARM64)
                 └─ scp → Kria A53
```

---

## 3. New Files

| File | Purpose |
|---|---|
| `cmake/toolchains/aarch64-linux-gnu.cmake` | CMake toolchain file — sets cross-compiler (via wrappers, see §4), multiarch sysroot, ARM64 hardening flags, and `-I` for cross-built dep headers. |
| `libEGD-master/hack/docker/Dockerfile.builder.arm64` | Docker builder image with the aarch64 cross-compiler and the cross-built static deps. |
| `libEGD-master/hack/docker/cross-wrappers/aarch64-linux-gnu-gcc-wrapper` | Thin shell wrapper that filters x86-only flags (`-fcf-protection=full`, `-mshstk`) out of the command line before exec-ing the real `aarch64-linux-gnu-gcc`. Required because libEGD's `CMakeLists.txt` appends those x86 CET flags *after* the toolchain runs — see §4. |
| `libEGD-master/hack/docker/cross-wrappers/aarch64-linux-gnu-g++-wrapper` | Same idea for C++. |
| `scripts/build_libegd_arm64.sh` | One-shot driver: builds the Docker image, runs cmake + make + install inside the container, copies example binaries out, verifies that `libEGD.so` and `JsonPublishC` are `ARM aarch64` via `file(1)`. Supports `--clean` and `--help`. |
| `.gitattributes` | Pins LF line endings on `*.sh`, `*.cmake`, `Dockerfile*` and `cross-wrappers/*` so a Windows checkout (`core.autocrlf=true`) cannot poison the Linux shebangs inside the builder container. |
| `.gitignore` (extended) | Adds `out/` and `.builder-*` so the reproducible build artefacts never get checked in. |

---

## 4. CMake Toolchain File + Compiler Wrappers

`cmake/toolchains/aarch64-linux-gnu.cmake` does four things:

1. Sets `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER` to the two **wrapper scripts** (`/usr/local/bin/aarch64-linux-gnu-{gcc,g++}-wrapper`) — see "Why a wrapper" below.
2. Restricts `find_library` / `find_path` to the ARM64 sysroot (`/usr/aarch64-linux-gnu`), `/usr/local/aarch64-linux-gnu`, and `/usr/lib/aarch64-linux-gnu` (Debian multiarch).
3. Adds `-I/usr/local/aarch64-linux-gnu/include` via `CMAKE_<LANG>_FLAGS_INIT` so libEGD's `#include <curl/curl.h>` resolves. libEGD's source has no `-I` for curl; the original x86 build worked only because the native Debian gcc searches `/usr/local/include` by default, which the cross gcc does not.
4. Sets a clean ARM64 hardening baseline — replaces `-fcf-protection=full -mshstk` (x86 CET) with `-mbranch-protection=standard` (ARM64 PAC+BTI equivalent on GCC 9+):

```cmake
set(ARM64_HARDENING "-D_FORTIFY_SOURCE=2 -fpie -fstack-protector-all -mbranch-protection=standard")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s ${ARM64_HARDENING}" CACHE STRING "" FORCE)
```

### Why a wrapper, not the bare cross compiler

The above `CACHE … FORCE` is **not sufficient on its own**. libEGD's `CMakeLists.txt:18-19` does:

```cmake
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${RELEASE_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE   "${CMAKE_C_FLAGS_RELEASE} ${RELEASE_FLAGS}")
```

…where `RELEASE_FLAGS` contains `-fcf-protection=full -mshstk`. That append happens **after** the toolchain file runs, so the x86 CET flags still land on the command line and `aarch64-linux-gnu-gcc` rejects `-mshstk` outright (`error: unrecognized command-line option '-mshstk'`).

The wrappers in `libEGD-master/hack/docker/cross-wrappers/` filter `-fcf-protection*` and `-mshstk`/`-mno-shstk` out of `$@` and `exec` the real `aarch64-linux-gnu-{gcc,g++}`. This lets us build Release **without patching GE-owned libEGD source**, which §2's "avoids modifying GE's libEGD source" promise demanded.

---

## 5. Dependency Build Order (inside Dockerfile.builder.arm64)

Order matters because libcurl links against libpsl, libnghttp2 and OpenSSL.

```
1. OpenSSL 3.0      (libssl-dev:arm64 via `dpkg --add-architecture arm64`)
                    -- not source-built; we deliberately use the same OpenSSL
                       ABI Ubuntu 22.04+ ships on the Kria to avoid drift.
2. zlib             (zlib1g-dev:arm64 via the same multiarch path)
3. libpsl 0.21.0    --host=aarch64-linux-gnu --with-pic --enable-static --disable-shared
4. libnghttp2 1.40.0 --host=aarch64-linux-gnu --with-pic --enable-static --disable-shared --enable-lib-only
5. libcurl 7.67.0   --host=aarch64-linux-gnu --with-pic --enable-static --disable-shared
                    --with-ssl=/usr  --with-nghttp2=<CROSS_PREFIX>  --with-libpsl=<CROSS_PREFIX>
                    CPPFLAGS=-I<CROSS_PREFIX>/include  LDFLAGS="-L<CROSS_PREFIX>/lib -L/usr/lib/aarch64-linux-gnu"
```

Source-built libs install to `/usr/local/aarch64-linux-gnu/` (prefix). The Dockerfile then `find`s each produced `.a` at its actual install location and symlinks it into `/usr/local/lib/` so libEGD's hard-coded `set(STATIC_DEPS /usr/local/lib/libcurl.a /usr/local/lib/libnghttp2.a /usr/local/lib/libpsl.a)` resolves unchanged.

> The `find`-based symlink (instead of a fixed path) was needed because autotools libdir heuristics can land static archives in `lib/`, `lib64/`, or `lib/<multiarch>/` depending on the host triplet; the robust step in the Dockerfile prints a tree dump if anything is missing.

---

## 6. Automatic Build — Single Command

```bash
# From the protocol_translator root:
./scripts/build_libegd_arm64.sh
```

Prerequisite: Docker daemon reachable on the build host. On a Windows dev box this can be either Docker Desktop or `docker-ce` installed inside WSL2 (the route used during M2-C verification).

What this does:
1. Builds the `libegd-arm64:builder` Docker image (cross-compiler + all deps).
2. Runs cmake configure + build + install inside the container (libEGD source bind-mounted read-only, toolchain file bind-mounted read-only at `/opt/toolchain/aarch64-linux-gnu.cmake`).
3. Installs library + headers to `out/arm64/` and copies the non-installed example binaries (`JsonPublishC`, `RawBytes*` etc.) into `out/arm64/bin/`.
4. Verifies output with `file(1)`: fails with exit code 4 if `libEGD.so` or `JsonPublishC` are not `ARM aarch64`.

To clean everything:
```bash
./scripts/build_libegd_arm64.sh --clean   # removes out/arm64 and the libegd-arm64:builder image
```

Help:
```bash
./scripts/build_libegd_arm64.sh --help
```

---

## 7. Expected Output (verified)

```
out/arm64/
├── lib/
│   ├── libEGD.so        ← shared library (ARM64 ELF, ~1.2 MB)
│   └── libEGD.a         ← static library  (ARM64,     ~1.1 MB)
├── include/
│   └── egd/
│       ├── client/
│       │   ├── egd_client.h    json_client.h
│       │   ├── egd_client.hpp  json_client.hpp
│       └── util/
│           ├── egd_spec.h  egd_status.h  external/
└── bin/
    ├── ListAvailablePlugins      ← installed by libEGD's install(TARGETS …)
    ├── JsonPublishC      JsonPublishCPP
    ├── JsonSubscribeC    JsonSubscribeCPP
    ├── RawBytesPublishC  RawBytesPublishCPP
    └── RawBytesSubscribeC RawBytesSubscribeCPP   ← examples, copied out by build script
```

`file(1)` output observed on the verified build:

```
libEGD.so:            ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV),
                      dynamically linked, BuildID[sha1]=..., stripped
libEGD.a:             current ar archive
JsonPublishC:         ELF 64-bit LSB pie executable, ARM aarch64, version 1 (SYSV),
                      dynamically linked, interpreter /lib/ld-linux-aarch64.so.1,
                      BuildID[sha1]=..., for GNU/Linux 3.7.0, stripped
JsonSubscribeC:       (same as JsonPublishC)
ListAvailablePlugins: (same as JsonPublishC)
```

The `interpreter /lib/ld-linux-aarch64.so.1` and `for GNU/Linux 3.7.0` lines are the proof the example binaries will hand off to the Kria's standard aarch64 dynamic loader on Ubuntu 22.04 (kernel ≥ 5.x).

---

## 8. Integration into Bridge Application Build

When the bridge application's `CMakeLists.txt` is written (M2-D/M3), libEGD is consumed as a pre-built external dependency:

```cmake
# In the bridge application's CMakeLists.txt
set(LIBEGD_ROOT "${CMAKE_SOURCE_DIR}/out/arm64")

add_library(EGD::shared SHARED IMPORTED)
set_target_properties(EGD::shared PROPERTIES
    IMPORTED_LOCATION "${LIBEGD_ROOT}/lib/libEGD.so"
    INTERFACE_INCLUDE_DIRECTORIES "${LIBEGD_ROOT}/include"
)

target_link_libraries(protocol_translator PRIVATE EGD::shared pthread ssl z)
```

Build the bridge application for ARM64:
```bash
# Build libEGD first
./scripts/build_libegd_arm64.sh

# Then build the bridge app with the same toolchain file
cmake -S . -B build/arm64 \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/aarch64-linux-gnu.cmake \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build/arm64 --parallel
```

---

## 9. Smoke Test on A53 (Pending Hardware Access)

Once the Kria board is available, run:

```bash
# Copy artifacts to Kria
scp out/arm64/lib/libEGD.so ubuntu@<kria-ip>:/tmp/
scp out/arm64/bin/JsonPublishCPP ubuntu@<kria-ip>:/tmp/   # example binary

# On the Kria A53:
ssh ubuntu@<kria-ip>
LD_LIBRARY_PATH=/tmp /tmp/JsonPublishCPP
# Expected: connects to EGD config server or prints usage — no SIGILL / illegal instruction
```

**SIGILL on startup** would indicate an ABI mismatch or wrong toolchain — check `file` output on the binary matches the Kria's Ubuntu kernel ABI.

---

## 10. Known Gaps / Blockers

| Gap | Impact | Resolution |
|---|---|---|
| **Smoke test not yet run on A53 hardware** | Cannot confirm runtime ABI compatibility on the actual board. The host-side `file(1)` check passes — see §7. | Run §9 once the Kria board is accessible. |
| libcurl configure pulls arm64 OpenSSL via `--with-ssl=/usr` + `LDFLAGS=-L/usr/lib/aarch64-linux-gnu` | Will break if Debian multiarch layout changes (unlikely on bookworm). | Parameterise via pkg-config in a follow-up. |
| `-mbranch-protection=standard` requires GCC 9+ | Builder uses Debian bookworm's GCC 12 — fine. If the cross toolchain is ever pinned to older GCC, drop this flag. | Documented in the toolchain file. |
| Bridge application `CMakeLists.txt` not yet written | Integration step (§8) is a design sketch only. | Implement in M3. |
| libEGD's `CMakeLists.txt` will keep appending x86 CET flags on every Release configure | Cosmetically ugly — those flags exit through the wrappers. | If GE merges an arch-aware `CMakeLists.txt` upstream, drop the wrappers. |

---

*Sources: `libEGD-master/CMakeLists.txt` (STATIC_DEPS, RELEASE_FLAGS, library targets); `libEGD-master/Makefile` (ARCH, Docker build pattern); `libEGD-master/hack/docker/Dockerfile.builder` (dep versions and build flags)*
