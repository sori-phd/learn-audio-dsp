cmake_minimum_required(VERSION 3.0)
project(ML_AEC_TDC)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

# include(cmake/webrtc.cmake)
include_directories(
    ../src/modules/audio_processing/include
    ../src/modules/audio_processing
    ../src/api
    ../src/rtc_base
    ../src
    abseil-cpp
)

ADD_EXECUTABLE(ch5 main.cc wavwriter.cc wavreader.cc)

file(GLOB MY_LIBS "${CMAKE_SOURCE_DIR}/lib/*.lib")
target_link_libraries(ch5 PRIVATE ${MY_LIBS})

add_custom_command(
    TARGET ch5 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_SOURCE_DIR}/../../resource/woman_voice.wav
            ${CMAKE_CURRENT_BINARY_DIR}/woman_voice.wav
)

target_compile_definitions(ch5 PRIVATE WEBRTC_APM_DEBUG_DUMP=0)
target_compile_definitions(ch5 PRIVATE WEBRTC_WIN)
