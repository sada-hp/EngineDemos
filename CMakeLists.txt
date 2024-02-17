cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(DemoPrj)

set(COPY_PATH ${DemoPrj_SOURCE_DIR}/bin)

add_subdirectory(engine)
add_subdirectory(demo)