cmake_minimum_required(VERSION 3.21)

project(D3D12Sample LANGUAGES CXX)

set(SRC_FILES
    src/D3D12Sample.cpp
    src/D3D12Sample.h
    )

add_executable(${PROJECT_NAME}
    ${SRC_FILES})

source_group("\\" FILES ${SRC_FILES})

target_include_directories(${PROJECT_NAME} INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME}
    Immortal)

file(COPY ${WORKSPACE}/Assets DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${ASSIMP_SHARED} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${OPENCV_SHARED} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${MONO_SHARED}   DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
MESSAGE("The libraries has been copied into" ${CMAKE_CURRENT_BINARY_DIR} --> ${ASSIMP_SHARED} ${OPENCV_SHARED} ${MONO_SHARED})
