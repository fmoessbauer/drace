#
 # DRace, a dynamic data race detector
 #
 # Copyright 2018 Siemens AG
 #
 # Authors:
 #   Felix Moessbauer <felix.moessbauer@siemens.com>
 #
 # SPDX-License-Identifier: MIT
 #


set(SOURCES
    "src/Load_Save.cpp"
    "src/Report_Handler.cpp"
    "src/Executor.cpp"
    "src/Command_Handler.cpp"
    "qt/main.cpp"
    "qt/draceGUI.cpp"
    "qt/draceGUI.h"
    "qt/draceGUI.ui"
    "qt/About_Dialog.cpp"
    "qt/About_Dialog.ui"
    "qt/About_Dialog.h"
    "qt/Report_Config.cpp"
    "qt/Report_Config.h"
    "qt/Report_Config.ui"
)

set(GUI_TARGET "drace-gui")

cmake_minimum_required(VERSION 3.5)

project(${GUI_TARGET} LANGUAGES CXX)

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

    add_executable(${GUI_TARGET} WIN32 ${SOURCES})
    target_include_directories(${GUI_TARGET} PRIVATE "include")
    
    target_link_libraries(${GUI_TARGET} PRIVATE Qt5::Widgets)

    find_package(WindeployQt)

    if(WindeployQt_FOUND)
        windeployqt(${GUI_TARGET} bin)
    else()
        message(WARN "Could not find windeployqt. Please deploy manually!")
    endif()

    # just for convenience we copy the Qt dlls to the build folder
    # to be able to run the drace-gui from there
    add_custom_command(TARGET ${GUI_TARGET} POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Core> $<TARGET_FILE_DIR:drace-gui>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Gui> $<TARGET_FILE_DIR:drace-gui>
		COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Qt5::Widgets> $<TARGET_FILE_DIR:drace-gui> )

    # install the GUI executable itself (without Qt shared modules)
    install(TARGETS ${GUI_TARGET} DESTINATION bin)

    # append licensing information to global license file
    if(LIC_FILE)
        file(READ "LICENSE.md" LIC_FILE_QT)
        file(APPEND ${LIC_FILE} "${LIC_SEP}DRace GUI\n\n${LIC_FILE_QT}")
    endif()

endif()
