#ifndef XVECTOR_H
#define XVECTOR_H

#include <map>


template<class _Ty, class _Ax = std::allocator<_Ty>>


class xvector : public std::vector<_Ty, _Ax> {
    using std::vector<_Ty, _Ax>::vector;
};

#endif
