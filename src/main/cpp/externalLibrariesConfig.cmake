# SPDX-FileCopyrightText: Copyright 2026 zjab
# SPDX-License-Identifier: LGPL-2.1-or-later

cmake_minimum_required(VERSION 3.22.1)

string(REGEX MATCH "[0-9]*$" MY_ANDROID_VERSION ${ANDROID_PLATFORM})

if (${CMAKE_ANDROID_ARCH} STREQUAL "arm64")
    set(TOOLCHAIN_TARGET "aarch64-linux-android${MY_ANDROID_VERSION}")
elseif (${CMAKE_ANDROID_ARCH} STREQUAL "arm")
    set(TOOLCHAIN_TARGET "armv7a-linux-android${MY_ANDROID_VERSION}")
elseif (${CMAKE_ANDROID_ARCH} STREQUAL "x86")
    set(TOOLCHAIN_TARGET "i686-linux-android${MY_ANDROID_VERSION}")
elseif (${CMAKE_ANDROID_ARCH} STREQUAL "x86_64")
    set(TOOLCHAIN_TARGET "x86_64-linux-android${MY_ANDROID_VERSION}")
else ()
    message(FATAL_ERROR "Unknown Android architecture: ${CMAKE_ANDROID_ARCH}")
endif ()

# ============================================================
# enable SIMD instruction set according to cpu architecture
# ============================================================

# --- libwebp ---
set(LIBWEBP_DISABLE_FLAGS
    --disable-gl --disable-sdl
    --disable-png --disable-jpeg --disable-tiff --disable-gif --disable-wic --enable-swap-16bit-csp
)

if(${CMAKE_ANDROID_ARCH} STREQUAL "arm64")
    set(LIBWEBP_SIMD_FLAGS
        --disable-avx2 --disable-sse4.1 --disable-sse2
    )
elseif(${CMAKE_ANDROID_ARCH} STREQUAL "arm")
    set(LIBWEBP_SIMD_FLAGS
        --enable-neon --disable-avx2 --disable-sse4.1 --disable-sse2
    )
elseif(${CMAKE_ANDROID_ARCH} STREQUAL "x86")
    set(LIBWEBP_SIMD_FLAGS
        --enable-sse2 --disable-avx2 --disable-sse4.1
    )
elseif(${CMAKE_ANDROID_ARCH} STREQUAL "x86_64")
    set(LIBWEBP_SIMD_FLAGS
        --enable-sse4.1 --enable-avx2 --enable-sse2
    )
endif()

# --- libav(FFmpeg) ---
if(${CMAKE_ANDROID_ARCH} STREQUAL "arm64")
    set(FFMPEG_SIMD_FLAGS
        --enable-asm
        --enable-neon
        --disable-dotprod --disable-i8mm
        --disable-armv5te --disable-armv6 --disable-armv6t2
        --disable-inline-asm --disable-x86asm
        --disable-mipsdsp --disable-mipsdspr2 --disable-msa --disable-mipsfpu
        --disable-mmi --disable-lsx --disable-lasx --disable-rvv
        --disable-sse --disable-sse2 --disable-sse3 --disable-ssse3 --disable-sse4 --disable-sse42
        --disable-avx --disable-xop --disable-fma3 --disable-fma4 --disable-avx2 --disable-avx512 --disable-avx512icl
        --disable-aesni
    )
elseif(${CMAKE_ANDROID_ARCH} STREQUAL "arm")
    set(FFMPEG_SIMD_FLAGS
        --enable-asm
        --enable-neon
        --enable-armv6 --enable-armv6t2
        --disable-dotprod --disable-i8mm
        --disable-armv5te
        --disable-inline-asm --disable-x86asm
        --disable-mipsdsp --disable-mipsdspr2 --disable-msa --disable-mipsfpu
        --disable-mmi --disable-lsx --disable-lasx --disable-rvv
        --disable-sse --disable-sse2 --disable-sse3 --disable-ssse3 --disable-sse4 --disable-sse42
        --disable-avx --disable-xop --disable-fma3 --disable-fma4 --disable-avx2 --disable-avx512 --disable-avx512icl
        --disable-aesni
    )
elseif(${CMAKE_ANDROID_ARCH} STREQUAL "x86")
    set(FFMPEG_SIMD_FLAGS
        --enable-asm
        --enable-mmx --enable-mmxext
        --enable-sse --enable-sse2 --enable-sse3 --enable-ssse3
        --disable-sse4 --disable-sse42
        --disable-avx --disable-xop --disable-fma3 --disable-fma4 --disable-avx2 --disable-avx512 --disable-avx512icl
        --disable-aesni
        --disable-neon --disable-armv5te --disable-armv6 --disable-armv6t2 --disable-dotprod --disable-i8mm
        --disable-inline-asm --disable-x86asm
        --disable-mipsdsp --disable-mipsdspr2 --disable-msa --disable-mipsfpu
        --disable-mmi --disable-lsx --disable-lasx --disable-rvv
    )
elseif(${CMAKE_ANDROID_ARCH} STREQUAL "x86_64")
    set(FFMPEG_SIMD_FLAGS
        --enable-asm
        --enable-mmx --enable-mmxext
        --enable-sse --enable-sse2 --enable-sse3 --enable-ssse3
        --enable-sse4 --enable-sse42
        --enable-avx --enable-xop --enable-fma3 --enable-fma4 --enable-avx2
        --enable-avx512 --enable-avx512icl
        --enable-aesni
        --disable-neon --disable-armv5te --disable-armv6 --disable-armv6t2 --disable-dotprod --disable-i8mm
        --disable-inline-asm --disable-x86asm
        --disable-mipsdsp --disable-mipsdspr2 --disable-msa --disable-mipsfpu
        --disable-mmi --disable-lsx --disable-lasx --disable-rvv
    )
endif()

include(ExternalProject)

ExternalProject_Add(
        build_libwebp
        PREFIX libwebp
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/libwebp
        CONFIGURE_COMMAND export "CC=clang -target ${TOOLCHAIN_TARGET}" LD=ld "LDFLAGS=-Wl,-z,max-page-size=16384"
            STRIP=llvm-strip AR=llvm-ar NM=llvm-nm LINK=llvm-link OBJDUMP=llvm-objdump DLLTOOL=llvm-dlltool RANLIB=llvm-ranlib
            PATH=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin:/usr/bin:/bin
        COMMAND test -L src_link || ln -s <SOURCE_DIR> src_link
        COMMAND cd src_link && ./autogen.sh && cd ..
        COMMAND src_link/configure --prefix=<INSTALL_DIR>
            --enable-shared --disable-static --disable-libwebpdemux
            ${LIBWEBP_SIMD_FLAGS} ${LIBWEBP_DISABLE_FLAGS}
            --host=${TOOLCHAIN_TARGET} --with-sysroot=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot
        BUILD_COMMAND export PATH=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin:/usr/bin:/bin
        COMMAND make
        BUILD_ALWAYS TRUE
        INSTALL_COMMAND export PATH=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin:/usr/bin:/bin
        COMMAND make install
        COMMAND cd <INSTALL_DIR>/lib
        COMMAND llvm-strip libsharpyuv.so libwebpmux.so libwebp.so
        COMMAND cp libsharpyuv.so libwebpmux.so libwebp.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
)

ExternalProject_Get_Property(build_libwebp INSTALL_DIR)
set(LIBWEBP_INSTALL_DIR ${INSTALL_DIR})

# the configure script of ffmpeg assumes CC is the linker
ExternalProject_Add(
        build_libav
        PREFIX libav
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/libav
        CONFIGURE_COMMAND env PATH=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin:/usr/bin:/bin
            PKG_CONFIG_PATH=${LIBWEBP_INSTALL_DIR}/lib/pkgconfig
            <SOURCE_DIR>/configure
            --prefix=<INSTALL_DIR> --enable-cross-compile --target-os=android --arch=${CMAKE_ANDROID_ARCH} 
            --sysroot=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/sysroot
            "--cc=clang -target ${TOOLCHAIN_TARGET}" "--extra-cflags=-I${LIBWEBP_INSTALL_DIR}/include"
            "--cxx=clang++ -target ${TOOLCHAIN_TARGET}" "--extra-cxxflags=-I${LIBWEBP_INSTALL_DIR}/include"
            "--ld=clang -target ${TOOLCHAIN_TARGET}" "--extra-ldflags=-L${LIBWEBP_INSTALL_DIR}/lib -Wl,-z,max-page-size=16384"
            --nm=llvm-nm --ar=llvm-ar --pkg-config=pkg-config --strip=llvm-strip
            --enable-shared --disable-static --enable-small --disable-programs --disable-doc
            --disable-avdevice --disable-swresample --disable-avfilter --disable-network
            --disable-everything --enable-encoder=libwebp_anim --enable-decoder=mjpeg --enable-muxer=image2 
            --enable-demuxer=concat --enable-demuxer=image2 --enable-protocol=file
            --enable-libwebp --disable-amf --disable-audiotoolbox --disable-cuda-llvm --disable-cuvid
            --disable-d3d11va --disable-d3d12va --disable-dxva2 --disable-ffnvcodec
            --disable-libdrm --disable-nvdec --disable-nvenc --disable-v4l2-m2m --disable-vaapi
            --disable-vdpau --disable-videotoolbox --enable-vulkan
            ${FFMPEG_SIMD_FLAGS}
        BUILD_COMMAND env PATH=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin:/usr/bin:/bin make
        BUILD_ALWAYS TRUE
        INSTALL_COMMAND env PATH=${CMAKE_ANDROID_NDK}/toolchains/llvm/prebuilt/linux-x86_64/bin:/usr/bin:/bin make install
        COMMAND cd <INSTALL_DIR>/lib && cp libavutil.so libavformat.so libavcodec.so libswscale.so ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}
        DEPENDS build_libwebp
)

add_custom_target(
        externalLibraries
        COMMAND ""
        DEPENDS build_libwebp build_libav
)
