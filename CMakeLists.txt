cmake_minimum_required(VERSION 3.12)
project(tmss)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -std=c++11 -O0 -Wall -fPIC -fno-ident -ldl -D_GNU_SOURCE -D_REENTRNT -g -ggdb ")

set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_COMPILER /usr/local/bin/g++)

message(${PROJECT_SOURCE_DIR})
set(3RD_PARTY_DIR ${PROJECT_SOURCE_DIR}/3rdparty)

function(add3rdparty src_dir include_dir)
	message("-- Add 3rdparty: " ${src_dir} " , include: " ${include_dir})
	add_subdirectory(${3RD_PARTY_DIR}/${src_dir})
	include_directories(${3RD_PARTY_DIR}/${include_dir})
endfunction(add3rdparty)

add3rdparty(fmt fmt/include)
#add3rdparty(yaml-cpp yaml-cpp/include)
add3rdparty(spdlog spdlog/include)
#add3rdparty(srt-core srt-core/srtcore)

function(add_complied_system_lib target lib_path include_path) 
	message("-- Add static lib: ${target}, include: '${include_path}', from: '${lib_path}'")
	add_library(${target} STATIC IMPORTED)
    set_property(TARGET ${target} APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG) # 
    set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_NOCONFIG "${lib_path}")
	add_dependencies(${target} "build_${target}")
	include_directories(${include_path}
) 
endfunction(add_complied_system_lib)

#add_complied_system_lib(libssl /usr/local/ssl/lib/libssl.a /usr/local/ssl/include)
#add_complied_system_lib(libcrypto /usr/local/ssl/lib/libcrypto.a /usr/local/ssl/include)

function(add_complied_lib target lib_path include_path) 
	message("-- Add static lib: ${target}, include: '${include_path}', from: '${lib_path}'")
	add_library(${target} STATIC IMPORTED)
    set_property(TARGET ${target} APPEND PROPERTY IMPORTED_CONFIGURATIONS NOCONFIG) # 
    set_target_properties(${target} PROPERTIES IMPORTED_LOCATION_NOCONFIG "${3RD_PARTY_DIR}/${lib_path}")
	add_dependencies(${target} "build_${target}")
	include_directories(${3RD_PARTY_DIR}/${include_path}
) # 增加头文件 
endfunction(add_complied_lib) 

add_complied_lib(libst st-1.9-multiple-threads/libst.a st-1.9-multiple-threads/include)
add_complied_lib(libspdlog spdlog/libspdlog.a spdlog/include)
add_complied_lib(libsrt srt/libsrt.a srt/srtcore)
#add_complied_lib(libssl /usr/local/ssl/lib/libssl.a openssl/include)
#add_complied_lib(libcrypto /usr/local/ssl/libcrypto.a openssl/include)

add_complied_lib(libavformat FFmpeg/libavformat/libavformat.a FFmpeg)
add_complied_lib(libavdevice FFmpeg/libavdevice/libavdevice.a FFmpeg)
add_complied_lib(libavcodec FFmpeg/libavcodec/libavcodec.a FFmpeg)
add_complied_lib(libavfilter FFmpeg/libavfilter/libavfilter.a FFmpeg)
add_complied_lib(libavutil FFmpeg/libavutil/libavutil.a FFmpeg)
add_complied_lib(libswscale FFmpeg/libswscale/libswscale.a FFmpeg)
add_complied_lib(libswresample FFmpeg/libswresample/libswresample.a FFmpeg)


#include_directories(${3RD_PARTY_DIR}/st/include)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/media_source)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/cache)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/coroutine)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/defs)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/dll)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/io)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/log)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/net)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/base/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/format)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/format/base)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/format/raw)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/format/flv)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/format/ts)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/protocol)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/http)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/rtmp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/transport)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/transport/st_trans)


aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/cache SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/coroutine SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/defs SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/dll SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/io SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/log SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/net SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/util SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/format SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/format/base SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/format/raw SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/format/flv SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/format/ts SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/protocol SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/http SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/protocol/rtmp SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/transport SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/transport/st_trans SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/base/main SRCS)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/media_source SRCS)

message("src value ${SRCS}")

add_executable(tmss ${SRCS})

#set(libuser_src ${CMAKE_CURRENT_SOURCE_DIR}/media_source/media_source.cpp)

#SET_TARGET_PROPERTIES(user_shared PROPERTIES OUTPUT_NAME "user")
#SET_TARGET_PROPERTIES(user_shared PROPERTIES CLEAN_DIRECT_OUTPUT 1)
#SET_TARGET_PROPERTIES(user_shared PROPERTIES VERSION 1.1 SOVERSION 1)


TARGET_LINK_LIBRARIES(tmss
		libspdlog
        #yaml-cpp
        libsrt
        libst
        libavformat
        libavdevice
        libavcodec
        libavfilter
        libavutil
        libswscale
        libswresample
        pthread
        z)
