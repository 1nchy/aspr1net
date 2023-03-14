#ifndef _ASP_BUFFER_HPP_
#define _ASP_BUFFER_HPP_

// #include <sys/types.h>
#include <string>
#include "package.hpp"

// typedef std::string buffer_t;

struct buffer_t{
    // unsigned char *_buff;
    char *_buff; // _buff[_write_idx] = '\0', always
    size_t _size;
    size_t _read_idx = 0;
    size_t _write_idx = 0;

public:
    buffer_t();
    buffer_t(const buffer_t&) = default;
    virtual ~buffer_t();

    size_t readable_size() const;
    size_t writeable_size() const;

    void shrink();
    void extend(size_t _new_size);
    void ending();

    void reset();
    void write(const char* _s);
    void write(size_t _n);

    char* package_buff() const;
    int package_size() const;
};

#endif //_ASP_BUFFER_HPP_
