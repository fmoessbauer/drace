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
#ifndef _STACKTRAC_H_
#define _STACKTRAC_H_

#include "xvector.h"
#include <unordered_map>
//#include "tl_alloc.h"
#include <ipc/DrLock.h>
#include <boost/graph/adjacency_list.hpp>


template <class Allocator>
struct tl_list { };
namespace boost {
    template <class Alloc, class ValueType>
    struct container_gen<tl_list<Alloc>, ValueType>
    {
        typedef typename Alloc::template rebind<ValueType>::other Allocator;
        typedef std::list<ValueType, Allocator> type;
    };
}


class StackTrace {

    typedef boost::property<boost::vertex_name_t, size_t> VertexProperty;
    typedef boost::adjacency_list <boost::listS, boost::listS, boost::bidirectionalS, VertexProperty > stack_tree;


    ///holds var_address, pc, stack_length
    std::unordered_map<size_t, std::pair<size_t,
        stack_tree::vertex_descriptor>> read_write;

    ///holds to complete stack tree
    ///is needed to create the stack trace in case of a race
    stack_tree local_stack;

    ///holds the current stack element
    stack_tree::vertex_descriptor ce;

    std::list<size_t> make_trace(std::pair<size_t, stack_tree::vertex_descriptor> data)
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


public:
    StackTrace():ce(boost::add_vertex(0, local_stack)){}
    

    void cut_tree(stack_tree::vertex_descriptor to_cut) {

        while (boost::in_degree(to_cut, local_stack) != 0) {
            stack_tree::in_edge_iterator edge = (boost::in_edges(to_cut, local_stack)).first;
            stack_tree::vertex_descriptor lower_vertex = boost::source(*(edge), local_stack);
            cut_tree(lower_vertex);
        }
        boost::clear_vertex(to_cut, local_stack);
        boost::remove_vertex(to_cut, local_stack);
    }

    void clean() {
        bool delete_flag = true;
        for (auto it = local_stack.m_vertices.begin(); it != local_stack.m_vertices.end(); it++) {
            if (boost::in_degree(*it, local_stack) == 0 && *it != ce) {
                delete_flag = true;
                for (auto jt = read_write.begin(); jt != read_write.end(); jt++) {
                    if (jt->second.second == *it) {
                        delete_flag = false;
                        break;
                    }
                }
                if (delete_flag) {
                    boost::clear_vertex(*it, local_stack);
                    boost::remove_vertex(*it, local_stack);
                }
            }

        }
        
    }

    void pop_stack_element() {
        auto edge = boost::out_edges(ce, local_stack);
        ce = (boost::target(*(edge.first), local_stack));
    }

    void push_stack_element(size_t element) {
        auto temp = boost::add_vertex(VertexProperty(element), local_stack);
        boost::add_edge(temp, ce, local_stack);
        ce = temp;
    }

    ///when a var is written or read, it copies the stack and adds the pc of the
    ///r/w operation to be able to return the stack trace if a race was detected
    void set_read_write(size_t addr, size_t pc) {

        if (read_write.find(addr) == read_write.end()) {
            read_write.insert({ addr, {pc, ce} });
        }
        else {
            read_write.find(addr)->second = { pc, ce };
        }

    }

    ///returns a stack trace of a clock for handing it over to drace
    std::list<size_t> return_stack_trace(size_t address) {

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



    /*void clean(size_t address) {

    }*/
};
#endif
