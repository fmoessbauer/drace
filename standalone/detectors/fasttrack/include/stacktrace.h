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
#ifndef _STACKTRACE_H_
#define _STACKTRACE_H_


#include <unordered_map>
#include <boost/graph/adjacency_list.hpp>

class StackTrace {

    typedef boost::property<boost::vertex_name_t, size_t> VertexProperty;
    typedef boost::adjacency_list <boost::listS, boost::listS, boost::bidirectionalS, VertexProperty > stack_tree;

    ///holds var_address, pc, stack_length
    std::unordered_map<size_t, std::pair<size_t,
        stack_tree::vertex_descriptor>> read_write;

    ///holds to complete stack tree
    ///is needed to create the stack trace in case of a race
    ///leafs of the tree which do not have pointer pointing to them may be deleted 
    stack_tree local_stack;

    ///holds the current stack element
    stack_tree::vertex_descriptor ce;

    uint16_t pop_count = 0;

    std::list<size_t> make_trace(std::pair<size_t, stack_tree::vertex_descriptor> data);
    void clean();

public:

    StackTrace();

    void pop_stack_element();
    void push_stack_element(size_t element);

    ///when a var is written or read, it copies the stack and adds the pc of the
    ///r/w operation to be able to return the stack trace if a race was detected
    void set_read_write(size_t addr, size_t pc);

    ///returns a stack trace of a clock for handing it over to drace
    std::list<size_t> return_stack_trace(size_t address);
};
#endif
