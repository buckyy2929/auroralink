#!/usr/bin/env bash
# ============================================================================
# build_libegd_arm64.sh
#
# One-shot driver for M2-C: cross-compile libEGD for Cortex-A53 (Kria PS,
# aarch64) using the libegd-arm64:builder Docker image and the
# cmake/toolchains/aarch64-linux-gnu.cmake CMake toolchain file.
#
# Usage:
#   ./scripts/build_libegd_arm64.sh           # build everything
#   ./scripts/build_libegd_arm64.sh --clean   # tear down image + out/arm64
#   ./scripts/build_libegd_arm64.sh -h|--help # show help
#
# Output:
#   out/arm64/lib/libEGD.so     (ARM aarch64 ELF, shared)
#   out/arm64/lib/libEGD.a      (ARM aarch64, static)
#   out/arm64/include/egd/...   (public headers)
#   out/arm64/bin/...           (example binaries: JsonPublishC etc.)
#
# Exit codes:
#   0 success
#   1 misuse / missing prerequisite
#   2 docker build failed
#   3 cmake configure / build failed
#   4 output verification failed (artifacts missing or not ARM64 ELF)
# ============================================================================
set -euo pipefail

# --- Locate the workspace ----------------------------------------------------
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
LIBEGD_DIR="${REPO_ROOT}/libEGD-master"
TOOLCHAIN_FILE="${REPO_ROOT}/cmake/toolchains/aarch64-linux-gnu.cmake"
DOCKERFILE="${LIBEGD_DIR}/hack/docker/Dockerfile.builder.arm64"
OUT_DIR="${REPO_ROOT}/out/arm64"
BUILDER_IMAGE="libegd-arm64:builder"
CONTAINER_WORK="/home/edge/libEGD"
CONTAINER_OUT="/home/edge/out"

# --- Helpers -----------------------------------------------------------------
log()  { printf '\033[1;34m[build_libegd_arm64]\033[0m %s\n' "$*"; }
warn() { printf '\033[1;33m[build_libegd_arm64]\033[0m %s\n' "$*" >&2; }
# die "<message>" [<exit-code>]   -- only the message goes to stderr.
die()  { printf '\033[1;31m[build_libegd_arm64]\033[0m %s\n' "$1" >&2; exit "${2:-1}"; }

usage() {
    sed -n '2,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
}

# --- Argument parsing --------------------------------------------------------
CLEAN=0
while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean)     CLEAN=1; shift ;;
        -h|--help)   usage; exit 0 ;;
        *)           die "unknown argument: $1 (use --help)" 1 ;;
    esac
done

# --- Prerequisite checks -----------------------------------------------------
command -v docker >/dev/null 2>&1 \
    || die "docker is not on PATH. Install Docker Desktop / docker-ce first." 1
docker info >/dev/null 2>&1 \
    || die "docker daemon is not reachable. Start Docker Desktop and retry." 1

[[ -f "${DOCKERFILE}" ]]      || die "missing ${DOCKERFILE}" 1
[[ -f "${TOOLCHAIN_FILE}" ]]  || die "missing ${TOOLCHAIN_FILE}" 1
[[ -d "${LIBEGD_DIR}" ]]      || die "missing ${LIBEGD_DIR}" 1

# --- Clean mode --------------------------------------------------------------
if [[ "${CLEAN}" -eq 1 ]]; then
    log "removing ${OUT_DIR}"
    rm -rf "${OUT_DIR}"
    if docker image inspect "${BUILDER_IMAGE}" >/dev/null 2>&1; then
        log "removing docker image ${BUILDER_IMAGE}"
        docker image rm -f "${BUILDER_IMAGE}" >/dev/null
    else
        log "docker image ${BUILDER_IMAGE} not present, skipping"
    fi
    log "clean done"
    exit 0
fi

# --- Step 1: build the builder image -----------------------------------------
log "building docker image ${BUILDER_IMAGE} (this is slow the first time)"
# Build with the libEGD repo as the build context so that
# hack/docker/cross-wrappers/* and the GE root CA are reachable via COPY.
docker build \
    --pull \
    --platform linux/amd64 \
    -f "${DOCKERFILE}" \
    -t "${BUILDER_IMAGE}" \
    "${LIBEGD_DIR}" \
    || die "docker build failed for ${BUILDER_IMAGE}" 2

# --- Step 2: cmake configure + build inside the container --------------------
mkdir -p "${OUT_DIR}"

# Detect host UID/GID so artifacts in out/arm64 are owned by the caller
# rather than root. Only meaningful on Linux/macOS; on Windows (Git Bash /
# MSYS / Cygwin) `id -u` returns a synthetic UID that Docker Desktop's
# file-sharing layer doesn't honour, so we leave the container as root and
# let Docker Desktop translate ownership for us.
USER_ARG=""
case "$(uname -s)" in
    Linux|Darwin)
        USER_ARG="--user $(id -u):$(id -g)" ;;
    *)
        log "host is $(uname -s); skipping --user (Docker Desktop handles ownership)" ;;
esac

log "running cmake + make inside ${BUILDER_IMAGE}"
# libEGD's CMakeLists.txt only registers EGD-shared, EGD-static, and
# ListAvailablePlugins for install(). The Json*/RawBytes* example binaries
# are built but not installed, so we manually copy them out for smoke testing.
# shellcheck disable=SC2086  # we want USER_ARG to word-split when non-empty
docker run --rm \
    --platform linux/amd64 \
    ${USER_ARG} \
    -v "${LIBEGD_DIR}":${CONTAINER_WORK}:ro \
    -v "${TOOLCHAIN_FILE}":/opt/toolchain/aarch64-linux-gnu.cmake:ro \
    -v "${OUT_DIR}":${CONTAINER_OUT} \
    -w /tmp/build \
    "${BUILDER_IMAGE}" \
    bash -euxo pipefail -c '
        cmake \
            -S '"${CONTAINER_WORK}"' \
            -B /tmp/build \
            -DCMAKE_TOOLCHAIN_FILE=/opt/toolchain/aarch64-linux-gnu.cmake \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX='"${CONTAINER_OUT}"'
        cmake --build /tmp/build --parallel
        cmake --install /tmp/build
        mkdir -p '"${CONTAINER_OUT}"'/bin
        for bin in JsonPublishC JsonPublishCPP JsonSubscribeC JsonSubscribeCPP \
                   RawBytesPublishC RawBytesPublishCPP \
                   RawBytesSubscribeC RawBytesSubscribeCPP; do
            if [[ -f /tmp/build/$bin ]]; then
                cp /tmp/build/$bin '"${CONTAINER_OUT}"'/bin/
            fi
        done
    ' \
    || die "cmake/make inside container failed" 3

# --- Step 3: verify outputs are ARM64 ELF ------------------------------------
log "verifying artifacts in ${OUT_DIR}"

required=(
    "${OUT_DIR}/lib/libEGD.so"
    "${OUT_DIR}/lib/libEGD.a"
    "${OUT_DIR}/include/egd/client/egd_client.h"
    "${OUT_DIR}/bin/ListAvailablePlugins"
)
for f in "${required[@]}"; do
    [[ -e "$f" ]] || die "expected artifact missing: $f" 4
done

if command -v file >/dev/null 2>&1; then
    so_info="$(file -b "${OUT_DIR}/lib/libEGD.so")"
    log "file libEGD.so: ${so_info}"
    case "${so_info}" in
        *"ARM aarch64"*) : ;;
        *) die "libEGD.so is not ARM aarch64: ${so_info}" 4 ;;
    esac
    if [[ -f "${OUT_DIR}/bin/JsonPublishC" ]]; then
        bin_info="$(file -b "${OUT_DIR}/bin/JsonPublishC")"
        log "file JsonPublishC: ${bin_info}"
        case "${bin_info}" in
            *"ARM aarch64"*) : ;;
            *) die "JsonPublishC is not ARM aarch64: ${bin_info}" 4 ;;
        esac
    fi
else
    warn "'file' not installed on host; skipping ARM aarch64 check (CI/host concern only)"
fi

cat <<EOF

================================================================================
 libEGD ARM64 build complete.
   artifacts:     ${OUT_DIR}
   shared lib:    ${OUT_DIR}/lib/libEGD.so
   static lib:    ${OUT_DIR}/lib/libEGD.a
   headers:       ${OUT_DIR}/include/egd/
   bin (installed): ${OUT_DIR}/bin/ListAvailablePlugins
   bin (copied):    ${OUT_DIR}/bin/JsonPublishC, JsonSubscribeC, ...
 Next (M2-C smoke test, pending Kria):
   scp ${OUT_DIR}/lib/libEGD.so          ubuntu@<kria-ip>:/tmp/
   scp ${OUT_DIR}/bin/JsonPublishC       ubuntu@<kria-ip>:/tmp/
   ssh ubuntu@<kria-ip> 'LD_LIBRARY_PATH=/tmp /tmp/JsonPublishC'
   # Expected on a Kria with no EGD config server reachable:
   #   Error reading config(-1): NETWORK_ERROR
   # (which proves the binary RAN -- no SIGILL, no ABI mismatch.)
================================================================================
EOF
