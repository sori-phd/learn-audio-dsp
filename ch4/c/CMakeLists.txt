cmake_minimum_required(VERSION 3.0)
project(ML_AEC_TDC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -stdlib=libc++")

add_subdirectory(muFFT)

ADD_EXECUTABLE(ch4 main.c wavwriter.c wavreader.c)
target_link_libraries(ch4 muFFT)

add_custom_command(
    TARGET ch4 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/../../resource/woman_voice.wav
            ${CMAKE_CURRENT_BINARY_DIR}/woman_voice.wav
)