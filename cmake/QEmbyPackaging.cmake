include_guard(GLOBAL)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/qwindowkit/CMakeLists.txt")
    message(FATAL_ERROR
        "libs/qwindowkit is missing. Run: git submodule update --init --recursive")
endif()
