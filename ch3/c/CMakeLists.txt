cmake_minimum_required(VERSION 3.0)
project(ML_AEC_TDC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")

ADD_EXECUTABLE(ch3 main.c wavwriter.c wavreader.c)

add_custom_command(
    TARGET ch3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/../../resource/woman_voice.wav
            ${CMAKE_CURRENT_BINARY_DIR}/woman_voice.wav
)