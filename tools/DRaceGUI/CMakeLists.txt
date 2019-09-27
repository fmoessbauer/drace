cmake_minimum_required(VERSION 3.5)

project(drace-gui LANGUAGES CXX)
set(CMAKE_CXX_COMPILER "C:\\Program Files (x86)\\Microsoft Visual Studio\\2017\\Professional\\VC\\Tools\\MSVC\\14.16.27023\\bin\\Hostx64\\x64\\cl.exe")
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets REQUIRED)

add_executable(drace-gui
  main.cpp
  mainwindow.cpp
  mainwindow.h
  mainwindow.ui
  )

target_link_libraries(drace-gui PRIVATE Qt5::Widgets)
