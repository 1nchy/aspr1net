#ifndef _ASP_BUFFER_HPP_
#define _ASP_BUFFER_HPP_

// #include <sys/types.h>
#include <string>

// typedef std::string buffer_t;

struct buffer_t{
    // unsigned char *_buff;
    char *_buff;
    size_t _size;
    size_t _read_idx = 0;
    size_t _write_idx = 0;

public:
    buffer_t();
    virtual ~buffer_t();

    size_t readable_size() const;
    size_t writeable_size() const;

    void shrink();
    void extend(size_t _ext_size);

    void reset();
    void write(const char* _s);
};

#endif //_ASP_BUFFER_HPP_
