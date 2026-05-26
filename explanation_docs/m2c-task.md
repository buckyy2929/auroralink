M2-C — Cross-Compile libEGD for Cortex-A53 (Kria PS) — Full ExplanationM2-C — Cross-Compile libEGD for Cortex-A53 (Kria PS) — Full Explanation
1. What is M2-C asking for, in plain English?
The Kria board has two processors inside it:

PL (FPGA) — the hardware side (Aurora, FIFO) — already handled by the bitstream
PS (A53 CPU) — runs Linux/Ubuntu — this is where your bridge software will run
The bridge software (the thing you're ultimately building in M2-D) needs to:

Read data from Aurora (via the FIFO interrupt path — already understood from M2-A)
Send that data over EGD (Ethernet Global Data) protocol to a remote controller — using libEGD
libEGD is a C++ library that handles the EGD/UDP communication. It lives in 

libEGD-master/
 in your repo.

The problem: libEGD was written for a desktop Linux x86_64 machine. The Kria's A53 CPU is ARM64 (aarch64). Code compiled on your Windows/x86 dev machine won't run on the Kria. You need to compile libEGD for ARM64 — that's what "cross-compile" means.

Analogy: You write a letter in English (x86 code). The Kria only reads Urdu (ARM64 instructions). M2-C is the translation job.

2. What exactly is in the repo right now?
The libEGD library — 

libEGD-master/
libEGD-master/
├── CMakeLists.txt          ← The build recipe (tells CMake what to compile)
├── README.md               ← How to build it (Docker + Make is the official way)
├── Makefile                ← Wraps Docker commands
├── hack/
│   └── docker/
│       └── Dockerfile.builder  ← Lists ALL dependencies needed to build libEGD
├── src/
│   ├── egd/
│   │   ├── client/         ← Core: egd_client.cpp, json_client.cpp
│   │   ├── config/         ← XML config parsing
│   │   └── util/           ← JSON, pugixml, CxxUrl (embedded 3rd-party libs)
│   └── examples/
│       ├── json_publish.c      ← How to SEND EGD data (C version)
│       ├── json_subscribe.c    ← How to RECEIVE EGD data (C version)
│       ├── raw_bytes_publish.c
│       └── raw_bytes_subscribe.c
The standalone EGD test file — at repo root


EgdSend.c
 — a standalone EGD sender (doesn't use libEGD, uses raw sockets)


EgdTest.h
 — header with the EGD protocol constants
3. What are libEGD's dependencies? (The hard part of cross-compiling)
From 

Dockerfile.builder
, libEGD needs these to build:

Dependency	Why needed	Complexity
cmake (≥3.8)	Build system	Easy — install on dev machine
build-essential / g++	C++ compiler	Need aarch64 version for cross-compile
libssl-dev (OpenSSL)	HTTPS for config fetch	Must be ARM64 version
zlib1g-dev	Compression	Must be ARM64 version
libcurl (v7.67.0, custom build)	HTTP GET for EGD XML config	Must be ARM64 version
libnghttp2 (v1.40.0)	HTTP/2 support for curl	Must be ARM64 version
libpsl (v0.21.0)	Public Suffix List for curl	Must be ARM64 version
libssh2 (v1.9.0)	SSH support	Must be ARM64 version
pthread	Threading (Subscribe uses a thread)	Available on Linux ARM64
From 

CMakeLists.txt
, the final link command is:

cmake
target_link_libraries(EGD-shared
    /usr/local/lib/libcurl.a
    /usr/local/lib/libnghttp2.a
    /usr/local/lib/libpsl.a
    pthread ssl z)
So the output is: libEGD.so (shared) or libEGD.a (static)

4. The Three Compile Approaches (and which one to pick)
The M2-C task asks you to choose one approach and execute it:

Option A — Cross-compile on dev host (Windows/x86 Linux VM → ARM64)
Your Dev Machine (x86_64 Linux or WSL2)
         │
         │  aarch64-linux-gnu-g++ compiler
         │  + ARM64 sysroot
         ▼
libEGD.so  ← ARM64 binary, even though you're on x86
         │
         │  copy to Kria
         ▼
Kria A53 CPU runs it ✓
Pros: Don't need the board available to build
Cons: Complex — need ARM64 versions of all the dependencies above

Option B — Native compile directly on Kria board ✅ (Simplest)
SSH into Kria → install gcc/g++, cmake, libssl-dev, zlib → run cmake + make
Output: libEGD.so on the board itself
Pros: No cross-compilation hassle — the board is ARM64, you compile natively
Cons: Need the board available; slower compile on ARM CPU
This is the recommended approach for M2-C given you have a Kria board

Option C — Yocto/PetaLinux SDK
Only relevant if the Kria runs a Yocto-built OS (not Ubuntu). Since you're using Ubuntu on Kria, skip this.

5. Step-by-Step: What Actually Needs to Be Done (Option B — Native)
This is what M2-C expects you to execute and document:

bash
# Step 1: SSH into Kria (running Ubuntu)
ssh ubuntu@<kria-ip>
# Step 2: Install build tools
sudo apt-get update
sudo apt-get install -y cmake g++ libssl-dev zlib1g-dev \
    libcurl4-openssl-dev git build-essential
# Step 3: Copy libEGD-master to the board
# (from your dev machine)
scp -r libEGD-master/ ubuntu@<kria-ip>:~/
# Step 4: Build libEGD
cd ~/libEGD-master
mkdir build && cd build
cmake ..
make -j4
# Step 5: Verify the output
ls -la src/      # should show libEGD.so or libEGD.a
file libEGD.so   # should say: ELF 64-bit LSB shared object, ARM aarch64
# Step 6: Install (optional)
sudo make install
# installs to: /usr/local/lib/libEGD.so
#              /usr/local/include/egd/
# Step 7: Smoke test — build and run an example
cd ~/libEGD-master/build
./JsonPublishC    # or ./JsonSubscribeC
The smoke test result the task requires looks like:

# Expected output (even if config fetch fails — that's OK for a smoke test):
Error reading config(-1): NETWORK_ERROR
# This proves the binary RUNS on A53 — the library works
6. What the Deliverable File Must Contain
The task wants docs/milestone2/M2-C_cross_compile.md with:

Required Item	What to write
Exact commands used	All apt-get, cmake, make commands with flags
Toolchain versions	g++ --version, cmake --version output
Chosen approach	Option B (native) — and why
Artifact path / install prefix	/usr/local/lib/libEGD.so, /usr/local/include/egd/
Smoke test result	Command + actual output on A53
Known gaps / blockers	e.g., "libcurl version mismatch", "libssl1.1 not available on Ubuntu 22.04", etc.
7. The Key Challenge You'll Likely Hit
The 

Dockerfile.builder
 installs custom-built versions of curl, nghttp2, libpsl — not the standard distro packages. This is because libEGD needs them compiled with -fPIC (Position Independent Code) to link into a shared library.

The 

CMakeLists.txt
 line 70 hard-codes paths:

cmake
set(STATIC_DEPS /usr/local/lib/libcurl.a /usr/local/lib/libnghttp2.a /usr/local/lib/libpsl.a)
If your Kria's Ubuntu doesn't have these at /usr/local/lib/, the build will fail with undefined references. You'd need to either:

Build those dependencies from source on the Kria (following the Dockerfile steps manually), OR
Modify CMakeLists.txt to point to wherever apt-get installed them, OR
Use the simpler EgdSend.c (no library, raw sockets) as an alternative smoke test
8. The Simpler Alternative — EgdSend.c
If libEGD proves too hard to build, the task also mentions:

Build at least one example binary (e.g. json_publish or EgdSend linked against libEGD)



EgdSend.c
 at the repo root is a standalone file that uses raw BSD sockets (no libEGD needed). Compiling it on the Kria is as simple as:

bash
aarch64-linux-gnu-gcc EgdSend.c -o EgdSend -lm   # cross-compile
# OR on the Kria natively:
gcc EgdSend.c -o EgdSend -lm
file EgdSend   # → ELF 64-bit ARM aarch64  ✓
This would at least prove the EGD raw-socket approach works on A53, even if the full libEGD build has dependency issues — which is worth documenting as a known gap.

Summary — M2-C in One Paragraph
M2-C is about proving that the EGD communication library can run on the Kria's ARM64 processor. The library (

libEGD-master/
) was written for desktop Linux. It has C++ dependencies (curl, SSL, pugixml, JSON) that need to be built for ARM64. The task requires you to: pick a build approach (native compile on the Kria is simplest), install dependencies, run cmake + make, verify the output binary is ARM64, do a quick smoke test (run one example), and document all the exact commands, versions, and any blockers in docs/milestone2/M2-C_cross_compile.md. The tricky part is that CMakeLists.txt hard-codes paths to custom-built versions of libcurl/libnghttp2/libpsl — you'll likely need to either build those from source or patch the CMake file to use system libraries instead.
1. What is M2-C asking for, in plain English?
The Kria board has two processors inside it:

PL (FPGA) — the hardware side (Aurora, FIFO) — already handled by the bitstream
PS (A53 CPU) — runs Linux/Ubuntu — this is where your bridge software will run
The bridge software (the thing you're ultimately building in M2-D) needs to:

Read data from Aurora (via the FIFO interrupt path — already understood from M2-A)
Send that data over EGD (Ethernet Global Data) protocol to a remote controller — using libEGD
libEGD is a C++ library that handles the EGD/UDP communication. It lives in 

libEGD-master/
 in your repo.

The problem: libEGD was written for a desktop Linux x86_64 machine. The Kria's A53 CPU is ARM64 (aarch64). Code compiled on your Windows/x86 dev machine won't run on the Kria. You need to compile libEGD for ARM64 — that's what "cross-compile" means.

Analogy: You write a letter in English (x86 code). The Kria only reads Urdu (ARM64 instructions). M2-C is the translation job.

2. What exactly is in the repo right now?
The libEGD library — 

libEGD-master/
libEGD-master/
├── CMakeLists.txt          ← The build recipe (tells CMake what to compile)
├── README.md               ← How to build it (Docker + Make is the official way)
├── Makefile                ← Wraps Docker commands
├── hack/
│   └── docker/
│       └── Dockerfile.builder  ← Lists ALL dependencies needed to build libEGD
├── src/
│   ├── egd/
│   │   ├── client/         ← Core: egd_client.cpp, json_client.cpp
│   │   ├── config/         ← XML config parsing
│   │   └── util/           ← JSON, pugixml, CxxUrl (embedded 3rd-party libs)
│   └── examples/
│       ├── json_publish.c      ← How to SEND EGD data (C version)
│       ├── json_subscribe.c    ← How to RECEIVE EGD data (C version)
│       ├── raw_bytes_publish.c
│       └── raw_bytes_subscribe.c
The standalone EGD test file — at repo root


EgdSend.c
 — a standalone EGD sender (doesn't use libEGD, uses raw sockets)


EgdTest.h
 — header with the EGD protocol constants
3. What are libEGD's dependencies? (The hard part of cross-compiling)
From 

Dockerfile.builder
, libEGD needs these to build:

Dependency	Why needed	Complexity
cmake (≥3.8)	Build system	Easy — install on dev machine
build-essential / g++	C++ compiler	Need aarch64 version for cross-compile
libssl-dev (OpenSSL)	HTTPS for config fetch	Must be ARM64 version
zlib1g-dev	Compression	Must be ARM64 version
libcurl (v7.67.0, custom build)	HTTP GET for EGD XML config	Must be ARM64 version
libnghttp2 (v1.40.0)	HTTP/2 support for curl	Must be ARM64 version
libpsl (v0.21.0)	Public Suffix List for curl	Must be ARM64 version
libssh2 (v1.9.0)	SSH support	Must be ARM64 version
pthread	Threading (Subscribe uses a thread)	Available on Linux ARM64
From 

CMakeLists.txt
, the final link command is:

cmake
target_link_libraries(EGD-shared
    /usr/local/lib/libcurl.a
    /usr/local/lib/libnghttp2.a
    /usr/local/lib/libpsl.a
    pthread ssl z)
So the output is: libEGD.so (shared) or libEGD.a (static)

4. The Three Compile Approaches (and which one to pick)
The M2-C task asks you to choose one approach and execute it:

Option A — Cross-compile on dev host (Windows/x86 Linux VM → ARM64)
Your Dev Machine (x86_64 Linux or WSL2)
         │
         │  aarch64-linux-gnu-g++ compiler
         │  + ARM64 sysroot
         ▼
libEGD.so  ← ARM64 binary, even though you're on x86
         │
         │  copy to Kria
         ▼
Kria A53 CPU runs it ✓
Pros: Don't need the board available to build
Cons: Complex — need ARM64 versions of all the dependencies above

Option B — Native compile directly on Kria board ✅ (Simplest)
SSH into Kria → install gcc/g++, cmake, libssl-dev, zlib → run cmake + make
Output: libEGD.so on the board itself
Pros: No cross-compilation hassle — the board is ARM64, you compile natively
Cons: Need the board available; slower compile on ARM CPU
This is the recommended approach for M2-C given you have a Kria board

Option C — Yocto/PetaLinux SDK
Only relevant if the Kria runs a Yocto-built OS (not Ubuntu). Since you're using Ubuntu on Kria, skip this.

5. Step-by-Step: What Actually Needs to Be Done (Option B — Native)
This is what M2-C expects you to execute and document:

bash
# Step 1: SSH into Kria (running Ubuntu)
ssh ubuntu@<kria-ip>
# Step 2: Install build tools
sudo apt-get update
sudo apt-get install -y cmake g++ libssl-dev zlib1g-dev \
    libcurl4-openssl-dev git build-essential
# Step 3: Copy libEGD-master to the board
# (from your dev machine)
scp -r libEGD-master/ ubuntu@<kria-ip>:~/
# Step 4: Build libEGD
cd ~/libEGD-master
mkdir build && cd build
cmake ..
make -j4
# Step 5: Verify the output
ls -la src/      # should show libEGD.so or libEGD.a
file libEGD.so   # should say: ELF 64-bit LSB shared object, ARM aarch64
# Step 6: Install (optional)
sudo make install
# installs to: /usr/local/lib/libEGD.so
#              /usr/local/include/egd/
# Step 7: Smoke test — build and run an example
cd ~/libEGD-master/build
./JsonPublishC    # or ./JsonSubscribeC
The smoke test result the task requires looks like:

# Expected output (even if config fetch fails — that's OK for a smoke test):
Error reading config(-1): NETWORK_ERROR
# This proves the binary RUNS on A53 — the library works
6. What the Deliverable File Must Contain
The task wants docs/milestone2/M2-C_cross_compile.md with:

Required Item	What to write
Exact commands used	All apt-get, cmake, make commands with flags
Toolchain versions	g++ --version, cmake --version output
Chosen approach	Option B (native) — and why
Artifact path / install prefix	/usr/local/lib/libEGD.so, /usr/local/include/egd/
Smoke test result	Command + actual output on A53
Known gaps / blockers	e.g., "libcurl version mismatch", "libssl1.1 not available on Ubuntu 22.04", etc.
7. The Key Challenge You'll Likely Hit
The 

Dockerfile.builder
 installs custom-built versions of curl, nghttp2, libpsl — not the standard distro packages. This is because libEGD needs them compiled with -fPIC (Position Independent Code) to link into a shared library.

The 

CMakeLists.txt
 line 70 hard-codes paths:

cmake
set(STATIC_DEPS /usr/local/lib/libcurl.a /usr/local/lib/libnghttp2.a /usr/local/lib/libpsl.a)
If your Kria's Ubuntu doesn't have these at /usr/local/lib/, the build will fail with undefined references. You'd need to either:

Build those dependencies from source on the Kria (following the Dockerfile steps manually), OR
Modify CMakeLists.txt to point to wherever apt-get installed them, OR
Use the simpler EgdSend.c (no library, raw sockets) as an alternative smoke test
8. The Simpler Alternative — EgdSend.c
If libEGD proves too hard to build, the task also mentions:

Build at least one example binary (e.g. json_publish or EgdSend linked against libEGD)



EgdSend.c
 at the repo root is a standalone file that uses raw BSD sockets (no libEGD needed). Compiling it on the Kria is as simple as:

bash
aarch64-linux-gnu-gcc EgdSend.c -o EgdSend -lm   # cross-compile
# OR on the Kria natively:
gcc EgdSend.c -o EgdSend -lm
file EgdSend   # → ELF 64-bit ARM aarch64  ✓
This would at least prove the EGD raw-socket approach works on A53, even if the full libEGD build has dependency issues — which is worth documenting as a known gap.

Summary — M2-C in One Paragraph
M2-C is about proving that the EGD communication library can run on the Kria's ARM64 processor. The library (

libEGD-master/
) was written for desktop Linux. It has C++ dependencies (curl, SSL, pugixml, JSON) that need to be built for ARM64. The task requires you to: pick a build approach (native compile on the Kria is simplest), install dependencies, run cmake + make, verify the output binary is ARM64, do a quick smoke test (run one example), and document all the exact commands, versions, and any blockers in docs/milestone2/M2-C_cross_compile.md. The tricky part is that CMakeLists.txt hard-codes paths to custom-built versions of libcurl/libnghttp2/libpsl — you'll likely need to either build those from source or patch the CMake file to use system libraries instead.
