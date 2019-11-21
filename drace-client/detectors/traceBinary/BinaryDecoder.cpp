/*
 * DRace, a dynamic data race detector
 *
 * Copyright 2018 Siemens AG
 *
 * Authors:
 *   Felix Moessbauer <felix.moessbauer@siemens.com>
 *
 * SPDX-License-Identifier: MIT
 */

#include <iostream>
#include <fstream>

#include <ipc/ExtsanData.h>
#include "ConsoleOutput.h"
#include "DetectorOutput.h"

#define CONSOLE_OUTPUT false


int main(int argc, char** argv){

    #if CONSOLE_OUTPUT
        ConsoleOutput output;
    #else
        DetectorOutput output;
    #endif

    std::fstream file("trace.bin", std::ios::in | std::ios::binary);
    auto buf = std::make_shared<ipc::event::BufferEntry>();
    
    while(file.read(reinterpret_cast<char*>(buf.get()),sizeof(ipc::event::BufferEntry)).good()){
        output.makeOutput(buf);
    }

}
