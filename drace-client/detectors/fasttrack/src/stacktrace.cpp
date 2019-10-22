
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

#include "stacktrace.h"

std::list<size_t> StackTrace::make_trace(std::pair<size_t, stack_tree::vertex_descriptor> data)
{
    std::list<size_t> this_stack;

    stack_tree::vertex_descriptor act_item = data.second;

    this_stack.push_front(data.first);
    auto map = boost::get(boost::vertex_name_t(), local_stack);
    this_stack.push_front(map[act_item]);

    while (boost::out_degree(act_item, local_stack) != 0) {
        auto edge = boost::out_edges(act_item, local_stack);
        act_item = (boost::target(*(edge.first), local_stack));
        if (map[act_item] != 0) {
            this_stack.push_front(map[act_item]);
        }
    }

    return this_stack;
}


StackTrace::StackTrace() :ce(boost::add_vertex(0, local_stack)){}



void StackTrace::clean() {
    return;
//not working so far
    bool delete_flag, sth_was_deleted;
    do {
        sth_was_deleted = false;
        for (auto it = local_stack.m_vertices.begin(); it != local_stack.m_vertices.end(); it++) {
            
            //if and only if the vertex is not  the current element, and it has no in_edges must be a top of stack
            if (*it != ce && boost::in_degree(*it, local_stack) == 0) {
                delete_flag = true; 
                for (auto jt = read_write.begin(); jt != read_write.end(); jt++) {//step through the read_write list to make sure no element is refering to it
                    if (jt->second.second == *it) {
                        delete_flag = false;
                        break;
                    }
                }
                if (delete_flag) {
                    boost::clear_vertex(*it, local_stack);
                    boost::remove_vertex(*it, local_stack);
                    sth_was_deleted = true;
                }
            }
        }
    } while (sth_was_deleted);

}

void StackTrace::pop_stack_element() {
    auto edge = boost::out_edges(ce, local_stack);
    ce = (boost::target(*(edge.first), local_stack));
    
    pop_count++;
    if(pop_count > 1){
        pop_count = 0;
        clean();
    } 
}

void StackTrace::push_stack_element(size_t element) {
    stack_tree::vertex_descriptor tmp;

    if(boost::in_degree(ce, local_stack) > 0){
        auto in_edges = boost::in_edges(ce, local_stack);
        for(auto it = in_edges.first; it != in_edges.second; it++){
            //check here if such a node is already existant and use it if so
            tmp = boost::source(*it, local_stack);
            auto desc = boost::get(boost::vertex_name_t(), local_stack, tmp);
            
            if(element == desc){
                ce = tmp; //node is already there, use it
                return;
            }
        }
    }

    tmp = boost::add_vertex(VertexProperty(element), local_stack);
    boost::add_edge(tmp, ce, local_stack);
    ce = tmp;

}

///when a var is written or read, it copies the stack and adds the pc of the
///r/w operation to be able to return the stack trace if a race was detected
void StackTrace::set_read_write(size_t addr, size_t pc) {

    if (read_write.find(addr) == read_write.end()) {
        read_write.insert({ addr, {pc, ce} });
    }
    else {
        read_write.find(addr)->second = { pc, ce };
    }

}

///returns a stack trace of a clock for handing it over to drace
std::list<size_t> StackTrace::return_stack_trace(size_t address) {

    auto it = read_write.find(address);
    if (it != read_write.end()) {
        auto data = it->second;
        std::list<size_t> t = make_trace(data);
        return t;
    }
    else {
        //A read/write operation was not tracked correctly -> return empty stack trace
        return std::list<size_t>(0);
    }

}


