# Sourced by apply-patches.sh and revert-patches.sh.

# patch directory -> project path
PROJECTS="
frameworks_base:frameworks/base
frameworks_native:frameworks/native
hardware_libhardware:hardware/libhardware
packages_apps_Phone:packages/apps/Phone
"

# series_present REPO PATCH... : true if the working tree of REPO already
# contains the whole series. Checked by reverting it, newest first, in a
# throwaway index loaded with the working tree; the real index is untouched.
series_present() {
    local repo=$1 index patch
    shift
    index=$(mktemp)
    rm -f "$index"
    (
        cd "$repo" || exit 1
        export GIT_INDEX_FILE=$index
        git read-tree HEAD && git add -A . || exit 1
        for patch in $(echo "$@" | tr ' ' '\n' | sort -r); do
            git apply -R --cached "$patch" 2>/dev/null || exit 1
        done
    )
    local ret=$?
    rm -f "$index"
    return $ret
}
