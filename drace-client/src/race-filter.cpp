
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

#include "race-filter.h"
#include <fstream>
#include <iostream>
#include <algorithm>

drace::RaceFilter::RaceFilter(std::string filename){
    std::ifstream in_file(filename);
    std::string entry;
    
    //reads a file which has thread sanitizer suppression file fashion 
    if (in_file.is_open()) {
        while(std::getline(in_file, entry)){
            //ommit lines which start with a '#' -> comments
            if(entry[0] != '#'){
                //valid entries are like 'subject:function_name' -> race_tos:std::foo
                size_t pos = entry.find(':', 0);
                if(pos != std::string::npos){
                    //race_tos (top of stack is the only supported subject)
                    if(entry.substr(0, pos) == "race_tos"){
                        filter_list.push_back(entry.substr(pos+1));
                    }
                }
            }
        }
        in_file.close();
    }
}

bool drace::RaceFilter::check_suppress(const drace::race::DecoratedRace & race){

    //get the top stack elements
    const std::string& top1_sym = race.first.resolved_stack.back().sym_name;
    const std::string& top2_sym = race.second.resolved_stack.back().sym_name;

    if(std::find(filter_list.begin(), filter_list.end(), top1_sym) == filter_list.end() &&
        std::find(filter_list.begin(), filter_list.end(), top2_sym) == filter_list.end())
    {
        return false; //not in list -> do not suppress
    }

    return true; 
}

void drace::RaceFilter::print_list(){

    std::cout << "Suppressed TOS functions: " << std::endl;
    for(auto it = filter_list.begin(); it != filter_list.end(); ++it){
        std::cout << *it << std::endl;
    }
}