find_package(Qt6 REQUIRED COMPONENTS Test Core Gui)

function(uplink_test TARGET)
    cmake_parse_arguments("" "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${TARGET} ${_SOURCES})
    target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${TARGET} PRIVATE Qt6::Test Qt6::Core ${_LIBS})
    add_test(NAME ${TARGET} COMMAND ${TARGET})
endfunction()

uplink_test(tst_ircparser
    SOURCES
        tst_ircparser.cpp
        ${CMAKE_SOURCE_DIR}/src/irc/ircparser.cpp
)

uplink_test(tst_chatformat
    SOURCES
        tst_chatformat.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/chatrenderer.cpp
    LIBS Qt6::Gui
)

uplink_test(tst_config
    SOURCES
        tst_config.cpp
        ${CMAKE_SOURCE_DIR}/src/config/config.cpp
        ${CMAKE_SOURCE_DIR}/src/config/keychainhelper.cpp
    LIBS tomlplusplus::tomlplusplus Qt6Keychain::Qt6Keychain
)
target_include_directories(tst_config PRIVATE ${QTKEYCHAIN_COMPAT_INCLUDE})

uplink_test(tst_ignoretypes
    SOURCES
        tst_ignoretypes.cpp
)

# ---------------------------------------------------------------------------
# Fuzz target — two build modes:
#   Standalone replay (default): any compiler, reads files from argv
#   libFuzzer binary:            requires clang + UPLINK_BUILD_FUZZ=ON
# ---------------------------------------------------------------------------
option(UPLINK_BUILD_FUZZ "Build libFuzzer harness (requires clang)" OFF)

add_executable(fuzz_ircparser
    fuzz_ircparser.cpp
    ${CMAKE_SOURCE_DIR}/src/irc/ircparser.cpp
)
target_include_directories(fuzz_ircparser PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(fuzz_ircparser PRIVATE Qt6::Core)

if(UPLINK_BUILD_FUZZ)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "UPLINK_BUILD_FUZZ requires clang++ (CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID})")
    endif()
    target_compile_definitions(fuzz_ircparser PRIVATE UPLINK_LIBFUZZER)
    target_compile_options(fuzz_ircparser PRIVATE -fsanitize=fuzzer,address,undefined)
    target_link_options(fuzz_ircparser PRIVATE -fsanitize=fuzzer,address,undefined)
else()
    # Standalone driver — runs the corpus as a smoke test
    target_compile_definitions(fuzz_ircparser PRIVATE)
    add_test(NAME fuzz_ircparser_corpus
        COMMAND fuzz_ircparser ${CMAKE_SOURCE_DIR}/tests/corpus
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    )
endif()
