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
int client_t::write(const char* _s) {
    // package_t _p;
    // _p._head._length = strlen(_s);
    _w_buffer.write(_s);
    return anetWrite(_fd, _w_buffer._buff, _w_buffer.readable_size());
}
int client_t::read() {
    package_head_t _head;
    anetRead(_fd, (char*)&_head, sizeof(package_head_t));
    int _ret = anetRead(_fd, _r_buffer._buff, _head._length);
    _r_buffer._size = _ret;
    _r_buffer._read_idx = 0;
    _r_buffer._write_idx = _ret;
    return _ret;
}

// protected implement
bool client_t::check() const {
    if (_ip == nullptr || _port < 0 || _fd < 0) {
        return false;
    }
    return true;
}