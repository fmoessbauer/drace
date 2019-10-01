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

#include "Load_Save.h"

Load_Save::Load_Save()
{
}


bool Load_Save::save(std::string path, std::string content){
 /*   std::ofstream save_to_file;
    save_to_file.open(path);
    save_to_file << "####DRACE CONFIG FILE####";
    for(auto i = 0; i < command->size(); i++){
        save_to_file << (command[i]).toStdString() << std::endl;
    }
    save_to_file << report_converter_path.toStdString() << std::endl;
    save_to_file << report_name.toStdString() << std::endl;
    save_to_file.close();*/
    return true;
}

bool Load_Save::load(std::string path, std::string content){

 /*   std::ifstream load_file;
    load_file.open(path);
    std::vector<std::string> content;
    if(load_file.is_open()){
        for( std::string line; getline( load_file, line );){
            content.push_back(line);
        }
    }
    else{
        return;
    }
    load_file.close();

    if(content[0] != "####DRACE CONFIG FILE####"){
        return;
    }*/
    return true;
}
