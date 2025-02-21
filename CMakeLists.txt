#cmake -S . -B .
#make
cmake_minimum_required(VERSION 3.16)
project(httpserver VERSION 1.0.0 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)
qt_standard_project_setup()
add_executable(httpserver
  main.cpp
  httpserver.cpp
)
target_link_libraries(httpserver PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::Network
)