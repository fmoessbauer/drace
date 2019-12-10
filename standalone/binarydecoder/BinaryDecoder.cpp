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
#ifndef BINARY_DECODER_H
#define BINARY_DECODER_H

#include <iostream>
#include <fstream>

#include <ipc/ExtsanData.h>
#include <clipp.h>
#include "DetectorOutput.h"


int main(int argc, char** argv){

//    std::string detec = "drace.detector.tsan.dll";
    std::string detec = "drace.detector.fasttrack.standalone.dll";
    std::string file = "trace.bin";
    
   
    auto cli = clipp::group(
        (clipp::option("-d", "--detector") & clipp::value("detector", detec)) % ("race detector (default: " + detec + ")"),
        (clipp::option("-f", "--filename") & clipp::value("filename", file)) % ("filename (default: " + file + ")")
    );
    if (!clipp::parse(argc, (char**)argv, cli)) {
        std::cout << clipp::make_man_page(cli, argv[0]) << std::endl;
        return -1;
    }

    std::cout << "Detector: " << detec.c_str() << std::endl;
    DetectorOutput output(detec.c_str());

   

    std::ifstream in_file(file, std::ios::binary | std::ios::ate);
    std::streamsize size = in_file.tellg();
    in_file.seekg(0, std::ios::beg);

    std::vector<ipc::event::BufferEntry> buffer((size_t)(size/sizeof(ipc::event::BufferEntry)));
   
    //ipc::event::BufferEntry buf;
    //int i = 0;
    
    if(in_file.read((char*)(buffer.data()), size).good())
    {
        for(auto it = buffer.begin(); it != buffer.end(); ++it){
            ipc::event::BufferEntry tmp = *it;
            output.makeOutput(&tmp);
        }
    }

}
#endif