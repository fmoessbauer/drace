
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
#include <regex>



drace::RaceFilter::RaceFilter(std::istream &content){
    init(content);
}


drace::RaceFilter::RaceFilter(std::string filename){
    std::ifstream f;
    f.open(filename);
    if (f.is_open()) {
        //reads a file which has thread sanitizer suppression file fashion 
        init(f);
    }
    else{
        //some log message
    }
    f.close();
}

void drace::RaceFilter::init(std::istream &content){
    std::string entry;
    while(std::getline(content, entry)){
        //omit lines which start with a '#' -> comments
        if(entry[0] != '#'){
            //valid entries are like 'subject:function_name' -> race_tos:std::foo
            size_t pos = entry.find(':', 0);
            if(pos != std::string::npos){
               
               //race_tos top of stack
                if(entry.substr(0, pos) == "race_tos"){
                    std::string tmp = entry.substr(pos+1);
                    normalize_string(tmp);
                    rtos_list.push_back(tmp);
                }

               //race
                if(entry.substr(0, pos) == "race"){
                    std::string tmp = entry.substr(pos+1);
                    normalize_string(tmp);
                    race_list.push_back(tmp);
                }
            }
        }
    }
}

void drace::RaceFilter::normalize_string(std::string & expr){
    size_t pos = 0;

    //make tsan wildcard * to regex wildcard .*
    pos = expr.find('*', 0);
    while(pos != std::string::npos){
        expr.insert(pos, ".");
        pos = expr.find('*', (pos+2));
    }

    //wild card at beginning, when no '^' and no existing wild card
    if(expr[0] != '^'){
        if((expr[0] != '.' && expr[1] != '*')){ expr.insert(0, ".*"); }
    }
    else{ expr.erase(0, 1); }

    //wild card at end, when no '$' and no existing wild card
    if(expr.back() != '$'){
        if(expr.back() != '*') { expr.append(".*"); }
    }
    else{ expr.pop_back(); }
}

bool drace::RaceFilter::check_suppress(const drace::race::DecoratedRace & race){

    if(check_race(race) || check_rtos(race)){
        return true;
    }
    return false; //not in list -> do not suppress
}

bool drace::RaceFilter::check_rtos(const drace::race::DecoratedRace & race){
    //get the top stack elements
    const std::string& top1_sym = race.first.resolved_stack.back().sym_name;
    const std::string& top2_sym = race.second.resolved_stack.back().sym_name;
    for(auto it = rtos_list.begin(); it != rtos_list.end(); ++it){
        std::regex expr(*it);
        std::smatch m;
        if( std::regex_match(top1_sym.begin(), top1_sym.end(), m, expr) ||
            std::regex_match(top2_sym.begin(), top2_sym.end(), m, expr) ){
                return true; //at least one regex matched -> suppress
        }
    }
    return false;
}

bool drace::RaceFilter::check_race(const drace::race::DecoratedRace & race){
    //get the top stack elements
    auto stack1 =  race.first.resolved_stack;
    auto stack2 = race.second.resolved_stack;
    
    for(auto it = race_list.begin(); it != race_list.end(); ++it){
        std::regex expr(*it);
        std::smatch m;

       for(auto st = stack1.begin(); st != stack1.end(); st++){
            const std::string& sym = st->sym_name;
            if( std::regex_match(sym.begin(), sym.end(), m, expr)) {
                return true; //regex matched -> suppress
            }
        }
        for(auto st = stack2.begin(); st != stack2.end(); st++){
            const std::string& sym = st->sym_name;
            if( std::regex_match(sym.begin(), sym.end(), m, expr)) {
                return true; //regex matched -> suppress
            }
        }
    }
    return false;
}

void drace::RaceFilter::print_list(){

    std::cout << "Suppressed TOS functions: " << std::endl;
    for(auto it = rtos_list.begin(); it != rtos_list.end(); ++it){
        std::cout << *it << std::endl;
    }
    std::cout << "Suppressed functions: " << std::endl;
    for(auto it = race_list.begin(); it != race_list.end(); ++it){
        std::cout << *it << std::endl;
    }
}