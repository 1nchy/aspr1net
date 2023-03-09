#ifndef _ASP_CLIENT_HPP_
#define _ASP_CLIENT_HPP_

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
    char _err_info[1024];

public:
    client_t() = default;
    // client_t(char* _ip, int _port);
    virtual ~client_t();

    int connect() { return connect(_ip, _port); }
    int connect(char* _ip, int _port);
    int write(const char* _s);
    int read();

protected:
    bool check() const;
};


#endif // _ASP_CLIENT_HPP_