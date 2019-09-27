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
#include "dr_api.h"

template <class T>
class tl_alloc
{
public:
    using value_type = T;
    void* context;

    tl_alloc() noexcept :context(dr_get_current_drcontext) {}  // not required, unless used

    template <class U> tl_alloc(tl_alloc<U> const&) noexcept {}

    value_type*  allocate(std::size_t n)  // Use pointer if pointer is not a value_type*
        
    {
        return reinterpret_cast<value_type*> (dr_thread_alloc(context, n * sizeof(value_type)));
    }

    void
        deallocate(value_type* p, std::size_t n) noexcept  // Use pointer if pointer is not a value_type*
    {
        dr_thread_free(context, p, n * sizeof(value_type));
    }

};

template <class T, class U>
bool
operator==(tl_alloc<T> const&, tl_alloc<U> const&) noexcept
{
    return true;
}

template <class T, class U>
bool
operator!=(tl_alloc<T> const& x, tl_alloc<U> const& y) noexcept
{
    return !(x == y);
}
