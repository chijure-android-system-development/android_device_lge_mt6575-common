#!/bin/bash
# Removes the MT6575 LG platform patches applied by apply-patches.sh, newest
# first, leaving the CM10 projects as they were. Projects whose series is
# not applied are skipped.
#
# Usage, from the top of the tree:  device/lge/mt6575-common/patches/revert-patches.sh

PATCH_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOP="$(cd "$PATCH_DIR/../../../.." && pwd)"
. "$PATCH_DIR/projects.sh"

for entry in $PROJECTS; do
    dir=${entry%%:*}
    repo=${entry#*:}
    patches=$(ls "$PATCH_DIR/$dir"/*.patch | sort)

    if ! series_present "$TOP/$repo" $patches; then
        echo "absent    $repo"
        continue
    fi

    for patch in $(echo $patches | tr ' ' '\n' | sort -r); do
        git -C "$TOP/$repo" apply -R --whitespace=nowarn "$patch" || exit 1
        echo "reverted  $repo/$(basename "$patch")"
    done
done
