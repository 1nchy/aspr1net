#include "client.hpp"

#include <unistd.h>

#include "anet.h"
#include "package.hpp"
#include "utils/log_utils.hpp"

// client_t::client_t(char* _ip, int _port) {
//     this->_fd = anetTcpConnect(_err_info, _ip, _port);
// }
client_t::~client_t() {
    if (check()) {
        close(_fd);
    }
}

int client_t::connect(char* _ip, int _port) {
    _fd = anetTcpConnect(_err_info, _ip, _port);
    if (_fd == -1) {
        ASP_ERR("connect error: %s\n", _err_info);
        return -1;
    }
    return _fd;
}
int client_t::send(const char* _s) {
    // package_t _p;
    // _p._head._length = strlen(_s);
    _w_buffer.write(_s);
    return this->flush();
}
int client_t::recv() {
    package_head_t _head;
    anetRead(_fd, (char*)&_head, sizeof(package_head_t));
    int _ret = anetRead(_fd, _r_buffer._buff, _head._length);
    _r_buffer._read_idx = 0;
    _r_buffer._write_idx = _ret;
    return _ret;
}
int client_t::file_send(const char* _file_name) {
    char _command_str[1024];
    sprintf(_command_str, "file_send %s", _file_name);
    _w_buffer.write(_command_str);
    FILE* _input_file = fopen(_file_name, "r");
    _buffer_list.clear();
    while (!feof(_input_file)) {
        std::shared_ptr<buffer_t> _b = std::make_shared<buffer_t>();
        int _read_n = fread(_b->package_buff(), 1, _b->package_size(), _input_file);
        package_head_t _head;
        _head._length = _read_n;
        memcpy(_b->_buff, &_head, sizeof(package_head_t));
        _b->_write_idx = _read_n + sizeof(package_head_t);
        _b->_buff[_b->_write_idx] = '\0';
        _buffer_list.push_back(_b);
    }
    std::shared_ptr<buffer_t> _bn = std::make_shared<buffer_t>();
    _bn->write(_buffer_list.size());
    _buffer_list.push_front(_bn);
    fclose(_input_file);
    return this->flush();
}
int client_t::file_recv(const char* _file_name) {
    char _command_str[1024];
    sprintf(_command_str, "file_recv %s", _file_name);
    send(_command_str);
    FILE* _output_file = fopen(_file_name, "w+");
    size_t _num = recv_integer();
    while (_num--) {
        if (recv() == 0) {
            break;
        }
        fwrite(_r_buffer._buff, _r_buffer.readable_size(), 1, _output_file);
    }
    fclose(_output_file);
    return 0;
}

// protected implement
size_t client_t::flush() {
    size_t _ret = 0;
    _ret += anetWrite(_fd, (char*)_w_buffer._buff, _w_buffer.readable_size());
    for (auto _b = _buffer_list.cbegin(); _b != _buffer_list.cend(); ++_b) {
        _ret += anetWrite(_fd, (char*)(*_b)->_buff, (*_b)->readable_size());
    }
    _w_buffer.reset();
    _buffer_list.clear();
    return _ret;
}
size_t client_t::recv_integer() {
    package_head_t _head;
    anetRead(_fd, (char*)&_head, sizeof(package_head_t));
    size_t _ret = 0;
    anetRead(_fd, (char*)&_ret, _head._length);
    return _ret;
}
bool client_t::check() const {
    if (_ip == nullptr || _port < 0 || _fd < 0) {
        return false;
    }
    return true;
}