#ifndef _ASP_CLIENT_HPP_
#define _ASP_CLIENT_HPP_

#include <list>
#include <memory>
#include <unordered_map>

#include "ae.hpp"
#include "arguments.hpp"
#include "buffer.hpp"

struct client_t;
struct command_t;

struct client_t {
    char* _ip = nullptr;
    int _port = -1;
    int _fd = -1;
    buffer_t _r_buffer;
    buffer_t _w_buffer;
    std::list<std::shared_ptr<buffer_t>> _buffer_list;
    arg_t _arg;
    char _err_info[1024];

    static const std::unordered_map<std::string, command_t> _s_command_table;

public:
    client_t() = default;
    // client_t(char* _ip, int _port);
    virtual ~client_t();

    int connect() { return connect(_ip, _port); }
    int connect(const char* _ip, int _port);
    int send(const char* _s);
    int recv(); // receive one single package
    int file_send(const char* _file_name);
    int file_recv(const char* _file_name);

    int process(const char* _args);
    size_t flush_2_server(); // flush buffer to %_fd
    size_t flush_2_client();

protected:
    size_t recv_integer();
    bool check() const;
};

// command table
typedef int command_proc_t(client_t* _c);
struct command_t {
    // char* _name; // name of command
    command_proc_t* _proc; // implement of command
    int _arity; // number of argument (!= 0) (+n: argc = n; -n: argc >= n)
};

namespace client_command {
    int ping_command(client_t* _c);
    int get_command(client_t* _c);
    int set_command(client_t* _c);
    int file_send_command(client_t* _c);
    int file_recv_command(client_t* _c);
}


#endif // _ASP_CLIENT_HPP_