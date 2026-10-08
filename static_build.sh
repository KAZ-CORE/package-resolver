#!/bin/bash

source conf.sh


case "$arch_build_target" in
    aarch64|aarch64_be|alpha|amdgcn|arc|arceb|arm|armeb|avr|\
    bpfeb|bpfel|csky|ez80|hexagon|hppa|hppa64|kalimba|kvx|lanai|\
    loongarch32|loongarch64|m68k|m88k|microblaze|microblazeel|\
    mips|mipsel|mips64|mips64el|msp430|nvptx|nvptx64|or1k|\
    powerpc|powerpcle|powerpc64|powerpc64le|propeller|riscv32|\
    riscv32be|riscv64|riscv64be|s390x|sh|sheb|sparc|sparc64|\
    spork8|spirv32|spirv64|ve|wasm32|wasm64|\
    x86_16|x86|x86_64|xcore|xtensa|xtensaeb|native)
        ;;
    *)
        echo "Invalid architecture: $arch_build_target"
        exit 1
        ;;
esac


os_build_target="${os_build_target}" \
arch_build_target="${arch_build_target}" \
libc_build_target="${libc_build_target}" \
./get_needs.sh && ./setup_zig.sh || exit 1


rm -rf "${traget_output}"

mkdir -p "${traget_output}" || exit 1


path_zlib="$(find .need_build -maxdepth 1 -type d \
    -name "zlib_${arch_build_target}_*" -print -quit)"

path_liblzma="$(find .need_build -maxdepth 1 -type d \
    -name "liblzma_${arch_build_target}_*" -print -quit)"

path_libsolv="$(find .need_build -maxdepth 1 -type d \
    -name "libsolv_${arch_build_target}_*" -print -quit)"


lib_zlib="${path_zlib}/libz_${arch_build_target}.a"
lib_liblzma="${path_liblzma}/liblzma_${arch_build_target}.a"
lib_libsolv="${path_libsolv}/libsolv_${arch_build_target}.a"

lib_libsolvext="$(
    find "${path_libsolv}" -maxdepth 1 -type f \
        -name 'libsolvext*.a' -print -quit
)"


if [[ -z "${path_zlib}" || ! -f "${lib_zlib}" ]]; then
    echo "Missing zlib for architecture: ${arch_build_target}"
    exit 1
fi


if [[ -z "${path_liblzma}" || ! -f "${lib_liblzma}" ]]; then
    echo "Missing liblzma for architecture: ${arch_build_target}"
    exit 1
fi


if [[ -z "${path_libsolv}" || ! -f "${lib_libsolv}" ]]; then
    echo "Missing libsolv for architecture: ${arch_build_target}"
    exit 1
fi


if [[ -z "${lib_libsolvext}" || ! -f "${lib_libsolvext}" ]]; then
    echo "Missing libsolvext for architecture: ${arch_build_target}"
    exit 1
fi


OBJ_DIR="$(pwd)/.obj/${build_target}"
NAME_ELF="${traget_output}/package_resolver"


CC="${zigfile} cc -target ${build_target}"
LD="${zigfile} cc -target ${build_target}"


CC_FLAGS="-Isrc \
'-I${path_libsolv}/headers' \
'-I${path_liblzma}/headers' \
'-I${path_zlib}/headers'"


LD_FLAGS="-static \
'${lib_libsolvext}' \
'${lib_libsolv}' \
'${lib_liblzma}' \
'${lib_zlib}'"


make \
    OBJ_DIR="${OBJ_DIR}" \
    NAME_ELF="${NAME_ELF}" \
    CC="${CC}" \
    CC_FLAGS="${CC_FLAGS}" \
    LD="${LD}" \
    LD_FLAGS="${LD_FLAGS}" \
    "${NAME_ELF}" || exit 1


echo "Built ${NAME_ELF}"
