#include "server.hpp"
// #include "client.hpp"

#include <unistd.h>

#include "anet.h"
#include "package.hpp"

#include "utils/log_utils.hpp"

// static implement

static void write_event_handler(ae_event_loop* _l, int _fd, void* _d, int _mask) {
    server_t::client_t* _c = static_cast<server_t::client_t*>(_d);
    buffer_t* _w_buff = &_c->_w_buffer;
    int _data_size = _w_buff->readable_size();
    if (_data_size == 0) {
        delete _c;
        return;
    }
    int _write_n = anetWrite(_c->_fd, (char*)_w_buff->_buff, _data_size);
    if (_write_n > 0) {
        _w_buff->_read_idx += _write_n;
    }
    if (_w_buff->readable_size() == 0) {
        _c->_loop->del_file_event(_c->_fd, AE_WRITABLE);
    }
}
static void read_event_handler(ae_event_loop* _l, int _fd, void* _d, int _mask) {
    server_t::client_t* _c = static_cast<server_t::client_t*>(_d);
    buffer_t* _r_buff = &_c->_r_buffer;
    // _r_buff->extend(512);
    package_head_t _head;
    ssize_t _read_n;
    if (read(_fd, &_head, sizeof(package_head_t)) == 0) {
        goto err;
    }
    _read_n = read(_fd, _r_buff->_buff + _r_buff->_write_idx, _head._length);
    if (_read_n > 0) {
        _r_buff->_write_idx += _read_n;
        _r_buff->_buff[_r_buff->_write_idx] = '\0';
    }
    else if (_read_n == 0) {
        goto err;
    }
    server_t::process_input_buffer(_c);
    _l->add_file_event(_c->_fd, AE_WRITABLE, write_event_handler, _c);
    return;

err:
    delete _c;
    return;
}
static void accept_tcp_handler(ae_event_loop* _l, int _fd, void* _d, int _mask) {
    char _client_ip[64];
    int _client_port;
    server_t* _server = static_cast<server_t*>(_d);
    int _client_fd = anetTcpAccept(NULL, _fd, _client_ip, sizeof(_client_ip), &_client_port);
    if (_client_fd != -1) {
        anetNonBlock(NULL, _client_fd);
        anetEnableTcpNoDelay(NULL, _client_fd);
        server_t::client_t* _c = new server_t::client_t();
        _c->_fd = _client_fd;
        _c->_loop = _l;

        if (_l->add_file_event(_client_fd, AE_READABLE, read_event_handler, _c) == -1) {
            delete _c;
        }
    }
}

// const std::unordered_map<const char*, command_t> _s_command_table = {
const std::unordered_map<std::string, command_t> server_t::_s_command_table = {
    {"ping", {command::ping_command, 1}},
    {"get", {command::get_command, 2}},
    {"set", {command::set_command, 3}},
};

// class implement
server_t::server_t() {
    
};
server_t::~server_t() {
    delete _loop;
};
void server_t::init() {
    this->_loop = new ae_event_loop(this->_max_client_count);
    // _loop->add_time_event(1000, nullptr, nullptr, nullptr);
    this->_listen_fd = anetTcpServer(_err_info, _port, NULL, _tcp_backlog);
    if (_listen_fd != -1) {
        anetNonBlock(_err_info, _listen_fd);
    }
    if (_loop->add_file_event(_listen_fd, AE_READABLE, accept_tcp_handler, this) != -1) {
        char _connect_info[64];
        anetFormatSock(_listen_fd, _connect_info, sizeof(_connect_info));
    }
}
void server_t::wait() {
    _loop->run();
}
void server_t::process_input_buffer(client_t* _c) {
    _c->_arg.process(_c->_r_buffer._buff);
    server_t::process_command(_c);
}
void server_t::process_command(client_t* _c) {
    ASP_LOG("client input: %s\n", _c->_r_buffer._buff);
    // for (int i = 0; i < _c->_arg.argc(); ++i) {
    //     ASP_LOG("argv[%d] = %s\n", i, _c->_arg.argv(i).c_str());
    // }

    auto _it = server_t::_s_command_table.find(_c->_arg.argv(0));
    if (_it == server_t::_s_command_table.end()) {
        ASP_ERR("error in command: <%s> not found.", _c->_arg.argv(0).c_str());
        return;
    }
    const command_t& _command_detail = _it->second;
    // check arity
    if (_command_detail._arity > 0) {
        if (_c->_arg.argc() != _command_detail._arity) {
            goto command_arity_err;
        }
    }
    else {
        if (_c->_arg.argc() < _command_detail._arity) {
            goto command_arity_err;
        }
    }

    // invoke
    _command_detail._proc(_c);
    return;

command_arity_err:
    ASP_ERR("error in command: <%s> not found.", _c->_arg.argv(0).c_str());
}


server_t::client_t::~client_t() {
    if (_loop != nullptr) {
        _loop->del_file_event(_fd, AE_READABLE);
        _loop->del_file_event(_fd, AE_WRITABLE);
        close(_fd);
    }
}



// command table implement
namespace command {
void ping_command(server_t::client_t* _c) {
    _c->_w_buffer.write("pong.");
}
void get_command(server_t::client_t* _c) {

}
void set_command(server_t::client_t* _c) {

}
};