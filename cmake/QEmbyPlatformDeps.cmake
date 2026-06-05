include_guard(GLOBAL)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/libs/qwindowkit/CMakeLists.txt")
    message(FATAL_ERROR
        "libs/qwindowkit is missing. Run: git submodule update --init --recursive")
endif()

function(qemby_append_default_qt_prefixes)
    set(_qemby_qt_prefixes)

    if(QEMBY_QT_ROOT)
        list(APPEND _qemby_qt_prefixes "${QEMBY_QT_ROOT}")
    endif()
    if(DEFINED ENV{QTDIR} AND NOT "$ENV{QTDIR}" STREQUAL "")
        list(APPEND _qemby_qt_prefixes "$ENV{QTDIR}")
    endif()
    if(DEFINED ENV{Qt6_DIR} AND NOT "$ENV{Qt6_DIR}" STREQUAL "")
        get_filename_component(_qemby_qt6_dir "$ENV{Qt6_DIR}" DIRECTORY)
        get_filename_component(_qemby_qt6_prefix "${_qemby_qt6_dir}/../.." ABSOLUTE)
        list(APPEND _qemby_qt_prefixes "${_qemby_qt6_prefix}")
    endif()

    if(WIN32)
        list(APPEND _qemby_qt_prefixes "E:/Qt6/6.9.2/msvc2022_64")
    elseif(APPLE)
        list(APPEND _qemby_qt_prefixes "$ENV{HOME}/Qt/6.9.2/macos")
    endif()

    list(REMOVE_DUPLICATES _qemby_qt_prefixes)
    foreach(_qemby_prefix IN LISTS _qemby_qt_prefixes)
        if(_qemby_prefix AND EXISTS "${_qemby_prefix}")
            list(APPEND CMAKE_PREFIX_PATH "${_qemby_prefix}")
        endif()
    endforeach()

    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
endfunction()

function(qemby_link_libmpv target_name)
    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR "qemby_link_libmpv target does not exist: ${target_name}")
    endif()

    set(_qemby_mpv_root "${CMAKE_SOURCE_DIR}/libs/libmpv")

    if(WIN32)
        find_path(QEMBY_MPV_INCLUDE_DIR mpv/client.h
            PATHS "${_qemby_mpv_root}/include"
            NO_DEFAULT_PATH)
        find_library(QEMBY_MPV_LIBRARY
            NAMES mpv libmpv
            PATHS "${_qemby_mpv_root}/lib" "${_qemby_mpv_root}"
            NO_DEFAULT_PATH)

        if(NOT QEMBY_MPV_INCLUDE_DIR OR NOT QEMBY_MPV_LIBRARY)
            message(FATAL_ERROR
                "libmpv was not found under ${_qemby_mpv_root}. "
                "Install libmpv for Windows or restore libs/libmpv.")
        endif()

        target_include_directories("${target_name}" PRIVATE "${QEMBY_MPV_INCLUDE_DIR}")
        target_link_libraries("${target_name}" PRIVATE "${QEMBY_MPV_LIBRARY}")

        find_file(QEMBY_MPV_DLL
            NAMES mpv-2.dll libmpv-2.dll mpv.dll libmpv.dll
            PATHS "${_qemby_mpv_root}" "${_qemby_mpv_root}/bin"
            NO_DEFAULT_PATH)
        if(QEMBY_MPV_DLL)
            add_custom_command(TARGET "${target_name}" POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${QEMBY_MPV_DLL}" $<TARGET_FILE_DIR:${target_name}>
                COMMENT "Copying libmpv runtime library")
        endif()
        return()
    endif()

    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(QEMBY_MPV QUIET mpv)
    endif()

    if(QEMBY_MPV_FOUND)
        if(QEMBY_MPV_INCLUDE_DIRS)
            target_include_directories("${target_name}" PRIVATE ${QEMBY_MPV_INCLUDE_DIRS})
        endif()
        if(QEMBY_MPV_LIBRARY_DIRS)
            target_link_directories("${target_name}" PRIVATE ${QEMBY_MPV_LIBRARY_DIRS})
        endif()
        target_link_libraries("${target_name}" PRIVATE ${QEMBY_MPV_LIBRARIES})
        if(QEMBY_MPV_CFLAGS_OTHER)
            target_compile_options("${target_name}" PRIVATE ${QEMBY_MPV_CFLAGS_OTHER})
        endif()
        return()
    endif()

    find_path(QEMBY_MPV_INCLUDE_DIR mpv/client.h)
    find_library(QEMBY_MPV_LIBRARY NAMES mpv libmpv)
    if(NOT QEMBY_MPV_INCLUDE_DIR OR NOT QEMBY_MPV_LIBRARY)
        message(FATAL_ERROR
            "libmpv development files were not found. Install libmpv/mpv "
            "development package, or make pkg-config expose the 'mpv' module.")
    endif()

    target_include_directories("${target_name}" PRIVATE "${QEMBY_MPV_INCLUDE_DIR}")
    target_link_libraries("${target_name}" PRIVATE "${QEMBY_MPV_LIBRARY}")
endfunction()
