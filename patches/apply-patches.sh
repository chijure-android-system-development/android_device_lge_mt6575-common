#!/bin/bash
# Applies the MT6575 LG platform patches to a CM10 tree as uncommitted changes.
# They are temporary: each one should eventually move into the device tree
# as a HAL/wrapper, like audio, xlog and the camera fix did.
#
# Usage, from the top of the tree:  device/lge/mt6575-common/patches/apply-patches.sh
# A project whose whole series is already there is skipped; a series that
# neither applies nor is already there stops the script.

PATCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOP="$(cd "$PATCH_DIR/../../../.." && pwd)"
. "$PATCH_DIR/projects.sh"

for entry in $PROJECTS; do
    dir=${entry%%:*}
    repo=${entry#*:}
    patches=$(ls "$PATCH_DIR/$dir"/*.patch | sort)

    if series_present "$TOP/$repo" $patches; then
        echo "present   $repo ($(echo $patches | wc -w) patches)"
        continue
    fi

    for patch in $patches; do
        if ! git -C "$TOP/$repo" apply --whitespace=nowarn "$patch"; then
            echo "CONFLICT  $repo/$(basename "$patch")" >&2
            exit 1
        fi
        echo "applied   $repo/$(basename "$patch")"
    done
done
