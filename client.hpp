#ifndef _ASP_CLIENT_HPP_
#define _ASP_CLIENT_HPP_

#include <list>
#include <memory>

#include "ae.hpp"
#include "buffer.hpp"

struct server_t;
struct client_t;

struct client_t {
    char* _ip = nullptr;
    int _port = -1;
    int _fd = -1;
    buffer_t _r_buffer;
    buffer_t _w_buffer;
    std::list<std::shared_ptr<buffer_t>> _buffer_list;
    char _err_info[1024];

public:
    client_t() = default;
    // client_t(char* _ip, int _port);
    virtual ~client_t();

    int connect() { return connect(_ip, _port); }
    int connect(char* _ip, int _port);
    int send(const char* _s);
    int recv(); // receive one single package
    int file_send(const char* _file_name);
    int file_recv(const char* _file_name);

protected:
    size_t flush(); // flush buffer to %_fd
    size_t recv_integer();
    bool check() const;
};


#endif // _ASP_CLIENT_HPP_