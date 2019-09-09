#ifndef XMAP_H
#define XMAP_H

#include <map>

template<class _kTy, class _Ty, class _Cmp = std::less<_kTy>, class _Ax = std::allocator<std::pair<const _kTy, _Ty>>>


class xmap : public std::map<_kTy, _Ty, _Cmp, _Ax>{
    using std::map<_kTy, _Ty, _Cmp, _Ax>::map;
};

#endif
