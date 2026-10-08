#!/bin/bash
source conf.sh

if [[ -f "${zigfile}" && "$(${zigfile} version)" == "${full_version}" ]]; then
    echo "Pre-installed Zig $(${zigfile} version)"
    exit 0
fi

rm -rf "${name_archive}" "${name_folder}"

echo "Download ${addr_zig}"
if ! curl -o "${name_archive}" -# "${addr_zig}"; then
    echo "not can Download ${addr_zig}"
    exit 1
fi

if ! tar -xf "${name_archive}"; then
    echo "not can extract ${name_archive}"
    exit 1
fi

if ! mv "${full_name}" "${name_folder}"; then
    echo "not can rename ${full_name}"
    exit 1
fi

if ! mkdir -p "${home_zig}"; then
    echo "not can create ${home_zig} folder"
    exit 1
fi

if ! mv "${name_folder}" "${home_zig}"; then
    echo "not can mov ${name_folder} to path ${home_zig}"
    exit 1
fi

rm -rf "${name_archive}"

if [[ ! -f "${zigfile}" || "$(${zigfile} version)" != "${full_version}" ]]; then
    echo "Zig installation failed"
    exit 1
fi

echo "installed Zig $(${zigfile} version)"
