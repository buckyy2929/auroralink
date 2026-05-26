# M2-C — Cross-Compile libEGD for Cortex-A53 (Kria PS)

**Status:** Build pipeline designed and scripted. Smoke test on A53 hardware pending.  
**Sources:** `libEGD-master/CMakeLists.txt`, `libEGD-master/Makefile`, `libEGD-master/hack/docker/Dockerfile.builder`

---

## 1. Build System Analysis

The existing libEGD build system is Docker + CMake. Key facts relevant to ARM64:

| Finding | Impact |
|---|---|
| `ALL_ARCH := amd64` hardcoded in Makefile | ARM64 not supported out of the box |
| Deps (libcurl, libpsl, libnghttp2) built from source inside Docker | Must cross-compile deps too — can't use apt packages for static `.a` files |
| Release flags include `-fcf-protection=full -mshstk` | **x86-only (Intel CET).** These fail on ARM64. Must be overridden. |
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
| `cmake/toolchains/aarch64-linux-gnu.cmake` | CMake toolchain file — sets cross-compiler, sysroot, overrides x86 release flags |
| `libEGD-master/hack/docker/Dockerfile.builder.arm64` | Docker builder with ARM64 cross-compiler and cross-built deps |
| `scripts/build_libegd_arm64.sh` | Single-command automatic build script |

---

## 4. CMake Toolchain File

`cmake/toolchains/aarch64-linux-gnu.cmake` does three things:

1. Sets `CMAKE_C_COMPILER` / `CMAKE_CXX_COMPILER` to `aarch64-linux-gnu-gcc/g++`
2. Restricts `find_library` / `find_path` to the ARM64 sysroot (`/usr/aarch64-linux-gnu`)
3. **Overrides the broken release flags** — replaces `-fcf-protection=full -mshstk` (x86 CET) with `-mbranch-protection=standard` (ARM64 PAC+BTI equivalent on GCC 9+):

```cmake
set(ARM64_HARDENING "-D_FORTIFY_SOURCE=2 -fpie -Wl,-z,relro,-z,now -fstack-protector-all -mbranch-protection=standard")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -s ${ARM64_HARDENING}" CACHE STRING "" FORCE)
```

---

## 5. Dependency Build Order (inside Dockerfile.builder.arm64)

Each dep is configured with `--host=aarch64-linux-gnu --disable-shared --enable-static --with-pic`:

```
1. libpsl 0.21.0    (no deps)
2. libnghttp2 1.40.0 (no deps)
3. OpenSSL arm64    (via dpkg --add-architecture arm64 + apt)
4. libcurl 7.67.0   (links against arm64 OpenSSL)
```

All installed to `/usr/local/aarch64-linux-gnu/lib/`. Symlinked to `/usr/local/lib/` so the existing CMakeLists.txt hardcoded paths resolve correctly without modification.

---

## 6. Automatic Build — Single Command

```bash
# From the protocol_translator root:
./scripts/build_libegd_arm64.sh
```

What this does:
1. Builds the `libegd-arm64:builder` Docker image (cross-compiler + all deps)
2. Runs cmake + build inside the container (source mounted read-only)
3. Installs artifacts to `out/arm64/`
4. Verifies output with `file` to confirm ARM64 ELF

To clean everything:
```bash
./scripts/build_libegd_arm64.sh --clean
```

---

## 7. Expected Output

```
out/arm64/
├── lib/
│   ├── libEGD.so        ← shared library (ARM64 ELF)
│   └── libEGD.a         ← static library (ARM64)
└── include/
    └── egd/
        ├── client/
        │   ├── egd_client.h
        │   ├── json_client.h
        │   └── ...
        └── util/
            └── egd_spec.h
```

Expected `file` output for `libEGD.so`:
```
libEGD.so: ELF 64-bit LSB shared object, ARM aarch64, version 1 (SYSV), dynamically linked
```

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
| Smoke test not yet run on A53 hardware | Cannot confirm runtime ABI compatibility | Run once Kria board is accessible |
| libcurl configured `--with-ssl` path hardcoded to `/usr/lib/aarch64-linux-gnu` in Dockerfile | May fail if Debian multiarch layout changes | Parameterize or use pkg-config |
| `-mbranch-protection=standard` requires GCC 9+ | Builder uses Debian bookworm's GCC 12 — OK. If toolchain is ever pinned to older GCC, drop this flag. | Document GCC version requirement |
| Bridge application CMakeLists.txt not yet written | Integration step (§8) is a design sketch only | Implement in M3 |

---

*Sources: `libEGD-master/CMakeLists.txt` (STATIC_DEPS, RELEASE_FLAGS, library targets); `libEGD-master/Makefile` (ARCH, Docker build pattern); `libEGD-master/hack/docker/Dockerfile.builder` (dep versions and build flags)*
