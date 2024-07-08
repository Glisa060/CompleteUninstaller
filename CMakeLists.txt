cmake_minimum_required(VERSION 3.28)
project(CompleteUninstaller)

set(CMAKE_CXX_STANDARD 20)

add_executable(CompleteUninstaller main.cpp
        header/gui.h
        impl/gui.cpp
        header/util.h
        impl/util.cpp
)

target_link_libraries(CompleteUninstaller comctl32)

