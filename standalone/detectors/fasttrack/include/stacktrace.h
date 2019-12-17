#ifndef _STACKTRACE_H_
#define _STACKTRACE_H_
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

#include <list>
#include <parallel_hashmap/phmap.h>
#include <boost/graph/adjacency_list.hpp>

/**
 * \brief Implements a stack depot capable to store callstacks
 *        with references to particular nodes.
 */
class StackTrace {

    typedef boost::property<boost::vertex_name_t, size_t> VertexProperty;
    typedef boost::adjacency_list <boost::listS, boost::listS, boost::bidirectionalS, VertexProperty > StackTree;

    ///holds var_address, pc, stack_length
    phmap::flat_hash_map<size_t, std::pair<size_t,
        StackTree::vertex_descriptor>> _read_write;

    ///holds to complete stack tree
    ///is needed to create the stack trace in case of a race
    ///leafs of the tree which do not have pointer pointing to them may be deleted 
    StackTree _local_stack;

    ///holds the current stack element
    StackTree::vertex_descriptor _ce;

    uint16_t _pop_count = 0;

    /// re-construct a stack-trace from a bottom node to the root
    std::list<size_t> make_trace(std::pair<size_t, StackTree::vertex_descriptor> data) const;

    /**
     * \brief cleanup unreferenced nodes in callstack tree
     * \warning very expensive
     */
    void clean();

public:

    StackTrace();

    /// pop the last element from the stack
    void pop_stack_element();

    /// push a new element to the stack depot
    void push_stack_element(size_t element);

    ///when a var is written or read, it copies the stack and adds the pc of the
    ///r/w operation to be able to return the stack trace if a race was detected
    void set_read_write(size_t addr, size_t pc);

    ///returns a stack trace of a clock for handing it over to drace
    std::list<size_t> return_stack_trace(size_t address) const;
};
#endif
