# Version from the latest reachable git tag ("v0.1.0" -> "0.1.0"), so a
# release is just `git tag -a vX.Y.Z -m vX.Y.Z`, no source edit/commit.
# Built exactly at the tag: "0.1.0"; commits after it: the short hash of
# the commit is appended, "0.1.0-0566e34".
# Falls back to 0.0.0 without git, outside a checkout or with no tag
# reachable (e.g. a shallow clone without tags).
# Taken when qmake runs, like the git commit and the build time: rerun
# qmake after a commit or a tag (Qt Creator: Build > Run qmake).
# Generates version.h in the build dir from version.h.in (there \" is a
# quote: qmake drops plain quotes in QMAKE_SUBSTITUTES).

PC_GIT_HASH = $$system(git -C $$PC_SRC rev-parse --short HEAD 2>/dev/null)

PC_VERSION = 0.0.0
PC_GIT_TAG = $$system(git -C $$PC_SRC describe --tags --abbrev=0 2>/dev/null)
PC_GIT_TAG ~= s/^v//
contains(PC_GIT_TAG, "^[0-9]+(\\.[0-9]+)?(\\.[0-9]+)?(\\.[0-9]+)?$") {
    PC_VERSION = $$PC_GIT_TAG
} else:!isEmpty(PC_GIT_TAG) {
    warning("latest git tag '$$PC_GIT_TAG' is not a plain X.Y.Z version, using $$PC_VERSION")
}
!isEmpty(PC_GIT_HASH):!system(git -C $$PC_SRC describe --tags --exact-match HEAD >/dev/null 2>&1): \
    PC_VERSION = $${PC_VERSION}-$${PC_GIT_HASH}

# Short hash of the commit, "-dirty" with uncommitted changes
isEmpty(PC_GIT_HASH) {
    PC_GIT_COMMIT = unknown
} else {
    PC_GIT_COMMIT = $$PC_GIT_HASH
    !system(git -C $$PC_SRC diff-index --quiet HEAD --): PC_GIT_COMMIT = $${PC_GIT_COMMIT}-dirty
}

PC_BUILD_TIMESTAMP = $$system(date -u \"+%Y-%m-%d %H:%M:%S UTC\")

atn_version.input  = $$PC_SRC/version.h.in
atn_version.output = $$OUT_PWD/version.h
QMAKE_SUBSTITUTES += atn_version
INCLUDEPATH += $$OUT_PWD
