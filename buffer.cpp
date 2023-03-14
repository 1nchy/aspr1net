#include "buffer.hpp"

#include <cstdlib>
#include <cstring>
#include <cassert>

#include "define.h"
#include "package.hpp"

buffer_t::buffer_t() : _size(DEFAULT_BUFF_SIZE) {
    this->_buff = new std::remove_pointer_t<decltype(_buff)>[DEFAULT_BUFF_SIZE];
    this->_size = DEFAULT_BUFF_SIZE;
}
buffer_t::~buffer_t() {
    delete[] _buff;
    _buff = nullptr;
    _size = 0;
}
size_t buffer_t::readable_size() const {
    assert(_size >= _write_idx);
    assert(_read_idx <= _write_idx);
    return _write_idx - _read_idx;
}
size_t buffer_t::writeable_size() const {
    assert(_size >= _write_idx);
    assert(_read_idx <= _write_idx);
    return _size - _write_idx;
}
void buffer_t::shrink() {
    // if (_read_idx > DEFAULT_BUFF_SIZE) {
        size_t _len = readable_size();
        memmove(_buff, _buff + _read_idx, _len);
        _read_idx = 0;
        _write_idx = _len;
    // }
}
void buffer_t::extend(size_t _ext_size) {
    if (writeable_size() < _ext_size) {
        shrink();
        size_t _new_size = _size + _ext_size;
        auto _old_buff = _buff;
        _buff = new std::remove_pointer_t<decltype(_buff)>[_new_size];
        // std::move(_old_buff, _buff, _size);
        strncpy(_buff, _old_buff, _size);
        delete[] _old_buff;
        _size = _new_size;
    }
}
void buffer_t::ending() {
    if (writeable_size() != 0) {
        _buff[_write_idx] = '\0';
    }
}
void buffer_t::reset() {
    _write_idx = 0;
    _read_idx = 0;
}
void buffer_t::write(const char* _s) {
    reset();
    package_head_t _head;
    _head._length = strlen(_s);
    memcpy(_buff, &_head, sizeof(package_head_t));
    _write_idx += sizeof(package_head_t);
    if (_head._length + 1 > writeable_size()) {
        extend(_head._length + 1 - writeable_size());
    }
    memcpy(_buff + _write_idx, _s, _head._length);
    _write_idx += _head._length;
    _buff[_write_idx] = '\0';
}
void buffer_t::write(size_t _n) {
    reset();
    package_head_t _head;
    _head._length = sizeof(size_t);
    memcpy(_buff, &_head, sizeof(package_head_t));
    _write_idx += sizeof(package_head_t);
    if (_head._length + 1 > writeable_size()) {
        extend(_head._length + 1 - writeable_size());
    }
    *((size_t*)package_buff()) = _n;
    _write_idx += _head._length;
    _buff[_write_idx] = '\0';
}

char* buffer_t::package_buff() const {
    return _buff + sizeof(package_head_t);
}
int buffer_t::package_size() const {
    return _size - sizeof(package_head_t) - 1;
}