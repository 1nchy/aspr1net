#ifndef _ASP_SERVER_HPP_
#define _ASP_SERVER_HPP_

#include "ae.hpp"
#include "arguments.hpp"
#include "buffer.hpp"

struct server_t;
// struct client_t;

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


#endif // _ASP_SERVER_HPP_