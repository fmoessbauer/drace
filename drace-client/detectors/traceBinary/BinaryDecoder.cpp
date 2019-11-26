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
#include <clipp.h>


int main(int argc, char** argv){

    std::string detec = "drace.detector.fasttrack.standalone.dll";
    std::string filename = "trace.bin";
    
    auto cli = clipp::group(
        (clipp::option("-d", "--detector") & clipp::value("detector", detec)) % ("race detector (default: " + detec + ")")
        //(clipp::option("-f", "--filename") & clipp::value("filename", filename)) % ("filename (default: " + filename + ")")
    );
    if (!clipp::parse(argc, (char**)argv, cli)) {
        std::cout << clipp::make_man_page(cli, argv[0]) << std::endl;
        return -1;
    }

    std::cout << "Detector: " << detec << std::endl;
    DetectorOutput output(detec.c_str());

    std::fstream file(filename, std::ios::in | std::ios::binary);
    //std::vector<std::shared_ptr<ipc::event::BufferEntry>>  buf_v(5, std::make_shared<ipc::event::BufferEntry>());

    //read in chunks
    std::shared_ptr<ipc::event::BufferEntry> buf = std::make_shared<ipc::event::BufferEntry>();
    while(file.read(reinterpret_cast<char*>(buf.get()),sizeof(ipc::event::BufferEntry)).good()){

//    while(file.read(reinterpret_cast<char*>(buf_v.data()), 5*(sizeof(ipc::event::BufferEntry))).good()){
        output.makeOutput(buf);
    }

}
