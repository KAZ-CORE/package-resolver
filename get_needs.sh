#!/bin/bash
source conf.sh

addr_base="https://kazftp.ir"
addr_static_pool="${addr_base}/static_pool"

filename_packages=".packages.sh"
addr_packages="${addr_static_pool}/${filename_packages}"

need_build=".need_build"

get_url() {
    echo "Downloading ${1}"

    if ! curl -fL -# -o "${2}" "${1}"; then
        echo "not can download ${1}"
        exit 1
    fi
}


rm -f "${filename_packages}"

get_url "${addr_packages}" "${filename_packages}"

source "${filename_packages}" || exit 1


filename_zlib="zlib_${arch_build_target}_${version_zlib}(${version_build_zlib}).${format_archive_zlib}"
name_folder_zlib="zlib_${arch_build_target}_${version_zlib}(${version_build_zlib})"
addr_zlib="${addr_base}/${location_zlib}/${filename_zlib}"

filename_liblzma="liblzma_${arch_build_target}_${version_liblzma}(${version_build_liblzma}).${format_archive_liblzma}"
name_folder_liblzma="liblzma_${arch_build_target}_${version_liblzma}(${version_build_liblzma})"
addr_liblzma="${addr_base}/${location_liblzma}/${filename_liblzma}"

filename_libsolv="libsolv_${arch_build_target}_${version_libsolv}(${version_build_libsolv}).${format_archive_libsolv}"
name_folder_libsolv="libsolv_${arch_build_target}_${version_libsolv}(${version_build_libsolv})"
addr_libsolv="${addr_base}/${location_libsolv}/${filename_libsolv}"

mkdir -p "${need_build}" || exit 1
cd "${need_build}" || exit 1


if [[ ! -d "${name_folder_zlib}" ]]; then
    rm -rf zlib_"${arch_build_target}"_*
    get_url "${addr_zlib}" "${filename_zlib}"

    mkdir "${name_folder_zlib}" || exit 1

    if ! tar -xf "${filename_zlib}" -C "${name_folder_zlib}"; then
        rm -rf "${filename_zlib}" "${name_folder_zlib}"
        exit 1
    fi

    rm -f "${filename_zlib}"
fi

if [[ ! -d "${name_folder_liblzma}" ]]; then
    rm -rf liblzma_"${arch_build_target}"_*
    get_url "${addr_liblzma}" "${filename_liblzma}"

    mkdir "${name_folder_liblzma}" || exit 1

    if ! tar -xf "${filename_liblzma}" -C "${name_folder_liblzma}"; then
        rm -rf "${filename_liblzma}" "${name_folder_liblzma}"
        exit 1
    fi

    rm -f "${filename_liblzma}"

fi


if [[ ! -d "${name_folder_libsolv}" ]]; then
    rm -rf libsolv_"${arch_build_target}"_*
    get_url "${addr_libsolv}" "${filename_libsolv}"

    #echo "${name_folder_libsolv}"
    mkdir "${name_folder_libsolv}" || exit 1

    if ! tar -xf "${filename_libsolv}" -C "${name_folder_libsolv}"; then
        rm -rf "${filename_libsolv}" "${name_folder_libsolv}"
        exit 1
    fi

    rm -f "${filename_libsolv}"

fi

cd .. || exit 1
