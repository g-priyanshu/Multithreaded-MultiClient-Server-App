#cmake_minimum_required(VERSION 3.10)
#project(Pacman)
#
#set(CMAKE_CXX_STANDARD 17)
#
## First executable: Pacman
#add_executable(server src/main.cpp)
#
#add_executable(client src/client.cpp)
## Second executable: Client

cmake_minimum_required(VERSION 3.10)

# Project Name
project(Pacman)

# Set the C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set Boost installation path for MSYS2 UCRT64 environment
set(BOOST_ROOT "C:/msys64/ucrt64")
set(Boost_INCLUDE_DIR "${BOOST_ROOT}/include")
set(Boost_LIBRARY_DIR "${BOOST_ROOT}/lib")

# Find Boost Libraries
find_package(Boost REQUIRED COMPONENTS system)

if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost libraries not found!")
endif()

# Include Boost headers
include_directories(${Boost_INCLUDE_DIR})

# Add the server executable
add_executable(server src/main.cpp)
target_link_libraries(server ${Boost_LIBRARIES} ws2_32 mswsock)

# Add the client executable
add_executable(client src/client.cpp)
target_link_libraries(client ${Boost_LIBRARIES} ws2_32 mswsock)

# Output executable paths
set_target_properties(server PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set_target_properties(client PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

