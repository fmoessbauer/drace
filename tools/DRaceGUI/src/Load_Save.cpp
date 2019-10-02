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

bool Load_Save::set_data(std::vector<std::string> content, DRaceGUI *instance){
    return false;
}

std::string Load_Save::return_serialized_data(DRaceGUI *instance){
    return "truetrue";
}

bool Load_Save::save(std::string path, DRaceGUI* instance){

    std::string do_save = return_serialized_data(instance);

    if(do_save != ""){
        std::ofstream save_to_file;
        save_to_file.open(path);
        if(save_to_file.is_open()){
            save_to_file << do_save << std::endl;
            save_to_file.close();
            return true;
        }
        else{
            return false;
        }
    }
    return false;

}

bool Load_Save::load(std::string path, DRaceGUI* instance){

    std::ifstream load_file;
    load_file.open(path);
    std::vector<std::string> content;
    if(load_file.is_open()){
        for( std::string line; getline( load_file, line );){
            content.push_back(line);
        }
        load_file.close();
    }
    else{
        return false;
    }

    if(set_data(content, instance)){
        return true;
    }
    return false;
}
