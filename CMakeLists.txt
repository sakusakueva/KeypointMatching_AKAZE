cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

# Project Name
project(keypoint_matching)

# Set parameter
set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# Set compiler options
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")  # For release
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -fsanitize=address -fno-omit-frame-pointer -g -Wall")  # For debug
#set(CXX_LIBRARIES "stdc++fs")
if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8)
    set(CMAKE_CXX_COMPILER "g++-8")
endif ()

# Find Package
find_package(OpenCV REQUIRED)

# Set Include Directories
set(INCLUDE_DIRS
    ${OpenCV_INCLUDE_DIRS}
    ${SRC_DIR}
)

# Set CXX_LIBRARIES
set(LIBRARIES
    ${CXX_LIBRARIES}
    ${OpenCV_LIBRARIES}
)

# Set Source Code
set(SRCS
    ${SRC_DIR}/main.cpp
)

# Link
add_executable(${PROJECT_NAME} ${SRCS})
include_directories(${INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${LIBRARIES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# Display Message

message(STATUS "")
message(STATUS "~~~ OpenCV Information ~~~")
message(STATUS "${OpenCV_INCLUDE_DIRS};")
message(STATUS "${OpenCV_LIBRARIES};")
message(STATUS "~~~~~~~~~~~~~~~~~~~~~~~~~~")
message(STATUS "")

