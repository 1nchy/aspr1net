#ifndef _ASP_PACKAGE_HPP_
#define _ASP_PACKAGE_HPP_

#include <cstdlib>

struct package_head_t {
    size_t _length;
} __attribute__((packed));

struct package_t {
    package_head_t _head;
    char _data[0];
} __attribute__((packed));

#endif // _ASP_PACKAGE_HPP_