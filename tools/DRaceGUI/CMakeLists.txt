cmake_minimum_required(VERSION 3.5)

project(drace-gui LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH "C:\\Qt\\Qt5.13.1\\5.13.1")
if(NOT Qt5_DIR)
    set(Qt5_DIR "C:\\Qt\\Qt5.13.1\\5.13.1\\msvc2017_64\\lib\\cmake\\Qt5")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Widgets)

if(Qt5_FOUND)
    message(STATUS "Build Qt5 DRace GUI")
    add_executable(drace-gui
      main.cpp
      draceGUI.cpp
      draceGUI.h
      draceGUI.ui
      )
    
    target_link_libraries(drace-gui PRIVATE Qt5::Widgets)

    add_custom_command(TARGET drace-gui POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Core> $<TARGET_FILE_DIR:drace-gui>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE_DIR:drace-gui>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Widgets> $<TARGET_FILE_DIR:drace-gui> )

    # TODO: also copy license and add note in global license file
    install(TARGETS drace-gui DESTINATION bin)
    install(
        FILES $<TARGET_FILE:Qt5::Core>
        $<TARGET_FILE:Qt5::Gui>
        $<TARGET_FILE:Qt5::Widgets>
        DESTINATION bin)
endif()
