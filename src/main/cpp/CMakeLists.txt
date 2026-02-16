# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html.
# For more examples on how to use CMake, see https://github.com/android/ndk-samples.

# Sets the minimum CMake version required for this project.
cmake_minimum_required(VERSION 3.22.1)

# Declares the project name. The project name can be accessed via ${ PROJECT_NAME},
# Since this is the top level CMakeLists.txt, the project name is also accessible
# with ${CMAKE_PROJECT_NAME} (both CMake variables are in-sync within the top level
# build script scope).
project("av-helper")

include(ExternalProject)

set(externalLibraries_DIR ${CMAKE_CURRENT_SOURCE_DIR})
find_package(externalLibraries CONFIG REQUIRED)

include_directories(${CMAKE_CURRENT_BINARY_DIR}/libwebp/include ${CMAKE_CURRENT_BINARY_DIR}/libav/include)

ExternalProject_Add(
        main
        SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/av-helper
        CMAKE_ARGS
            -S<SOURCE_DIR>
            -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
            -DCMAKE_EXPORT_COMPILE_COMMANDS=${CMAKE_EXPORT_COMPILE_COMMANDS}
            -DCMAKE_SYSTEM_VERSION=${CMAKE_SYSTEM_VERSION}
            -DANDROID_PLATFORM=${ANDROID_PLATFORM}
            -DANDROID_ABI=${ANDROID_ABI}
            -DCMAKE_ANDROID_ARCH_ABI=${CMAKE_ANDROID_ARCH_ABI}
            -DANDROID_NDK=${ANDROID_NDK}
            -DCMAKE_ANDROID_NDK=${CMAKE_ANDROID_NDK}
            -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
            -DCMAKE_MAKE_PROGRAM=${CMAKE_MAKE_PROGRAM}
            -DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
            -DCMAKE_LIBRARY_OUTPUT_DIRECTORY=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
            -DCMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
            -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
            -B${CMAKE_CURRENT_BINARY_DIR}
            -GNinja
        DEPENDS externalLibraries
)

#add_custom_target(
#        ${CMAKE_PROJECT_NAME}
#        COMMAND exit 1
##        DEPENDS main
#)

add_library(${CMAKE_PROJECT_NAME} SHARED
        # List C/C++ source files with relative paths to this CMakeLists.txt.
        av-helper/av-helper.c
        av-helper/frame_queue.c
)

add_dependencies(${CMAKE_PROJECT_NAME} externalLibraries)
target_link_libraries(
        ${CMAKE_PROJECT_NAME}
        android
        log
        sharpyuv
        webp
        webpmux
        avformat
        avcodec
        avutil
        swscale
        -L${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)
