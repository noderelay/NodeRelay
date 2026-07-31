# Regenerates gitversion.h at build time so the embedded hash tracks HEAD
# without a reconfigure. Only rewrites the header when the hash changes.
# Script mode has no policy floor from the top-level CMakeLists — without
# this, IN_LIST below breaks on CMakes that default CMP0057 to OLD.
cmake_minimum_required(VERSION 3.16)
# Only trust git if SRC_DIR is itself a checkout. A release tarball has no
# .git, but git walks up the tree — extracted inside another repo (an AUR
# clone, say) it would report that repo's HEAD as our build hash.
set(GIT_HASH "")
if(EXISTS "${SRC_DIR}/.git")
    execute_process(COMMAND git -C "${SRC_DIR}" rev-parse --short HEAD
                    OUTPUT_VARIABLE GIT_HASH
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                    RESULT_VARIABLE GIT_RC)
    if(NOT GIT_RC EQUAL 0)
        set(GIT_HASH "")
    endif()
endif()

if(GIT_HASH)
    execute_process(COMMAND git -C "${SRC_DIR}" diff-index --quiet HEAD --
                    ERROR_QUIET
                    RESULT_VARIABLE DIRTY_RC)
    # A clean tree sitting exactly on its own release tag IS the release —
    # the tag already pins the commit, so no hash suffix. Anything else
    # (past the tag, dirty, tag not fetched) keeps the hash for bug reports.
    execute_process(COMMAND git -C "${SRC_DIR}" tag --points-at HEAD
                    OUTPUT_VARIABLE HEAD_TAGS
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET)
    string(REPLACE "\n" ";" HEAD_TAGS "${HEAD_TAGS}")
    if(DIRTY_RC EQUAL 0 AND "v${BASE_VERSION}" IN_LIST HEAD_TAGS)
        set(VERSION_FULL "${BASE_VERSION}")
    else()
        if(NOT DIRTY_RC EQUAL 0)
            set(GIT_HASH "${GIT_HASH}.dev")
        endif()
        set(VERSION_FULL "${BASE_VERSION}+${GIT_HASH}")
    endif()
else()
    # Not a git checkout (release tarball) — fall back to the plain version
    set(VERSION_FULL "${BASE_VERSION}")
endif()

file(WRITE "${OUT_FILE}.tmp"
"#pragma once

#define UPLINK_GIT_HASH     \"${GIT_HASH}\"
#define UPLINK_VERSION_FULL \"${VERSION_FULL}\"
")
execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${OUT_FILE}.tmp" "${OUT_FILE}")
file(REMOVE "${OUT_FILE}.tmp")
