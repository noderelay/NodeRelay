# Regenerates gitversion.h at build time so the embedded hash tracks HEAD
# without a reconfigure. Only rewrites the header when the hash changes.
execute_process(COMMAND git -C "${SRC_DIR}" rev-parse --short HEAD
                OUTPUT_VARIABLE GIT_HASH
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET
                RESULT_VARIABLE GIT_RC)
if(NOT GIT_RC EQUAL 0)
    set(GIT_HASH "")
endif()

if(GIT_HASH)
    execute_process(COMMAND git -C "${SRC_DIR}" diff-index --quiet HEAD --
                    ERROR_QUIET
                    RESULT_VARIABLE DIRTY_RC)
    if(NOT DIRTY_RC EQUAL 0)
        set(GIT_HASH "${GIT_HASH}.dirty")
    endif()
    set(VERSION_FULL "${BASE_VERSION}+${GIT_HASH}")
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
