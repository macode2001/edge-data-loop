cmake_minimum_required(VERSION 3.16)
project(edge_data_loop LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(edge_agent
    src/main.cpp
    src/cache_store.cpp
    src/http_client.cpp
    src/sensor_simulator.cpp
    src/uploader.cpp
)

target_include_directories(edge_agent PRIVATE src)

if(WIN32)
    target_link_libraries(edge_agent PRIVATE ws2_32)
endif()
