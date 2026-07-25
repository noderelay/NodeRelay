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
        keychainhelper_stub.cpp
    LIBS tomlplusplus::tomlplusplus
)

uplink_test(tst_logreader
    SOURCES
        tst_logreader.cpp
        ${CMAKE_SOURCE_DIR}/src/model/logreader.cpp
)

uplink_test(tst_ignoretypes
    SOURCES
        tst_ignoretypes.cpp
)

uplink_test(tst_panetree
    SOURCES
        tst_panetree.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/panetree.cpp
)

# ---------------------------------------------------------------------------
# Fuzz targets — two build modes:
#   Standalone replay (default): any compiler, replays corpus as a smoke test
#   libFuzzer binary:            requires clang + UPLINK_BUILD_FUZZ=ON
# ---------------------------------------------------------------------------
option(UPLINK_BUILD_FUZZ "Build libFuzzer harnesses (requires clang)" OFF)

if(UPLINK_BUILD_FUZZ AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "UPLINK_BUILD_FUZZ requires clang++ (CMAKE_CXX_COMPILER_ID=${CMAKE_CXX_COMPILER_ID})")
endif()

function(uplink_fuzz TARGET)
    cmake_parse_arguments("" "" "CORPUS" "SOURCES;LIBS" ${ARGN})
    add_executable(${TARGET} ${_SOURCES})
    target_include_directories(${TARGET} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    target_link_libraries(${TARGET} PRIVATE Qt6::Core ${_LIBS})
    if(UPLINK_BUILD_FUZZ)
        target_compile_definitions(${TARGET} PRIVATE UPLINK_LIBFUZZER)
        target_compile_options(${TARGET} PRIVATE -fsanitize=fuzzer,address,undefined)
        target_link_options(${TARGET} PRIVATE -fsanitize=fuzzer,address,undefined)
    else()
        # Standalone driver — runs the corpus as a smoke test
        add_test(NAME ${TARGET}_corpus
            COMMAND ${TARGET} ${CMAKE_SOURCE_DIR}/tests/${_CORPUS}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )
    endif()
endfunction()

uplink_fuzz(fuzz_ircparser
    SOURCES
        fuzz_ircparser.cpp
        ${CMAKE_SOURCE_DIR}/src/irc/ircparser.cpp
    CORPUS corpus
)

uplink_fuzz(fuzz_chatformat
    SOURCES
        fuzz_chatformat.cpp
        ${CMAKE_SOURCE_DIR}/src/ui/chatrenderer.cpp
    LIBS Qt6::Gui
    CORPUS corpus_chatformat
)
