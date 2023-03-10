#ifndef _ASP_SERVER_HPP_
#define _ASP_SERVER_HPP_

#include "ae.hpp"
#include "arguments.hpp"
#include "buffer.hpp"

#include <string>
#include <unordered_map>

struct server_t;
struct command_t;

static void write_event_handler(ae_event_loop* _l, int _fd, void* _d, int _mask);
static void read_event_handler(ae_event_loop* _l, int _fd, void* _d, int _mask);
static void accept_tcp_handler(ae_event_loop* _l, int _fd, void* _d, int _mask);

struct server_t {
    struct client_t; // only used in server
    int _port;
    int _tcp_backlog;
    int _listen_fd;
    int _max_client_count;
    ae_event_loop* _loop = nullptr;
    char _err_info[1024];

    static const std::unordered_map<std::string, command_t> _s_command_table;

public:
    server_t();
    virtual ~server_t();
    void init();
    void wait();

    friend void write_event_handler(ae_event_loop* _l, int _fd, void* _d, int _mask);
    friend void read_event_handler(ae_event_loop* _l, int _fd, void* _d, int _mask);
    friend void accept_tcp_handler(ae_event_loop* _l, int _fd, void* _d, int _mask);

private:
    static void process_input_buffer(client_t* _c);
    static void process_command(client_t* _c);

public:
struct client_t {
    int _fd = -1;
    buffer_t _r_buffer;
    buffer_t _w_buffer;
    ae_event_loop* _loop = nullptr;
    arg_t _arg;

public:
    client_t() = default;
    virtual ~client_t();
    void process_buffer();
};
};

// command table
typedef void command_proc_t(server_t::client_t* _c);
struct command_t {
    // char* _name; // name of command
    command_proc_t* _proc; // implement of command
    int _arity; // number of argument (!= 0) (+n: argc = n; -n: argc >= n)
};
// extern const std::unordered_map<const char*, command_t> _s_command_table;
namespace command {
void ping_command(server_t::client_t* _c);
void get_command(server_t::client_t* _c);
void set_command(server_t::client_t* _c);
};

#endif // _ASP_SERVER_HPP_