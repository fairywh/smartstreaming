#!/bin/bash

PROJECT_DIR="."
THIRDPARTY_DIR="${PROJECT_DIR}/3rdparty"
cd ${THIRDPARTY_DIR}/spdlog/ && cmake ./ && make -j8
cd ../../
cd ${THIRDPARTY_DIR}/FFmpeg/ && ./configure && make -j8
cd ../../
cd ${THIRDPARTY_DIR}/srt/ && cmake ./ -DENABLE_ENCRYPTION=OFF -DENABLE_STDCXX_SYNC=ON && make -j8
cd ../../

mkdir build
cd build && cmake .. && make -j8

