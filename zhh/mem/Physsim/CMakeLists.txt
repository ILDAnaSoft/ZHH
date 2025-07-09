cmake_minimum_required(VERSION 3.15...3.29)
project(PhyssimWrapper LANGUAGES CXX)

set(PYBIND11_FINDPYTHON ON)
find_package(pybind11 CONFIG REQUIRED)

pybind11_add_module(PhyssimWrapper PhyssimWrapper.cpp)

find_package(ROOT REQUIRED COMPONENTS RIO Tree Physics)

find_package(Physsim REQUIRED)
target_link_libraries(PhyssimWrapper PRIVATE
    ${ROOT_LIBRARIES}
    Physsim::Physsim)

target_include_directories(PhyssimWrapper PRIVATE
    ${ROOT_INCLUDE_DIRS}
    ${PHYSSIM_INCLUDE_DIRS})