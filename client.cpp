#include "client.hpp"

#include <unistd.h>

#include "anet.h"
#include "package.hpp"
#include "utils/log_utils.hpp"

const std::unordered_map<std::string, command_t> client_t::_s_command_table = {
    {"ping", {client_command::ping_command, 1}},
    {"get", {client_command::get_command, 2}},
    {"set", {client_command::set_command, 3}},
    {"file_send", {client_command::file_send_command, 2}},
    {"file_recv", {client_command::file_recv_command, 2}},
};

// client_t::client_t(char* _ip, int _port) {
//     this->_fd = anetTcpConnect(_err_info, _ip, _port);
// }
client_t::~client_t() {
    if (check()) {
        close(_fd);
    }
}

int client_t::connect(const char* _ip, int _port) {
    _fd = anetTcpConnect(_err_info, _ip, _port);
    if (_fd == -1) {
        ASP_ERR("connect error: %s\n", _err_info);
        return -1;
    }
    anetBlock(_err_info, _fd);
    return _fd;
}
int client_t::send(const char* _s) {
    // package_t _p;
    // _p._head._length = strlen(_s);
    _w_buffer.write(_s);
    return this->flush_2_server();
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
    return this->flush_2_server();
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

int client_t::process(const char* _command_str) {
    _r_buffer.reset();
    _buffer_list.clear();
    _arg.process(_command_str);
    auto _it = client_t::_s_command_table.find(_arg.argv(0));
    if (_it == client_t::_s_command_table.end()) {
        ASP_ERR("error in command: <%s> not found.", _arg.argv(0).c_str());
        return -1;
    }
    const command_t& _command_detail = _it->second;
    // check arity
    if (_command_detail._arity > 0) {
        if (_arg.argc() != _command_detail._arity) {
            goto command_arity_err;
        }
    }
    else {
        if (_arg.argc() < _command_detail._arity) {
            goto command_arity_err;
        }
    }

    // invoke
    return _command_detail._proc(this);
command_arity_err:
    ASP_ERR("error in command: <%s> arity error.", _arg.argv(0).c_str());
    return -1;
}

// protected implement
size_t client_t::flush_2_server() {
    size_t _ret = 0;
    _ret += anetWrite(_fd, (char*)_w_buffer._buff, _w_buffer.readable_size());
    for (auto _b = _buffer_list.cbegin(); _b != _buffer_list.cend(); ++_b) {
        _ret += anetWrite(_fd, (char*)(*_b)->_buff, (*_b)->readable_size());
    }
    _w_buffer.reset();
    _buffer_list.clear();
    return _ret;
}
size_t client_t::flush_2_client() {
    int _max_len = _r_buffer.readable_size();
    return printf("%.*s\n", _max_len, _r_buffer._buff);
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


/// client_command implement
namespace client_command {
int ping_command(client_t* _c) {
    _c->send("ping");
    _c->recv();
    return _c->flush_2_client();
}
int get_command(client_t* _c) {
    return 0;
}
int set_command(client_t* _c) {
    return 0;
}
int file_send_command(client_t* _c) {
    _c->file_send(_c->_arg.argv(1).c_str());
    return 0;
}
int file_recv_command(client_t* _c) {
    _c->file_recv(_c->_arg.argv(1).c_str());
    return 0;
}
}