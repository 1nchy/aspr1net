#include "server.hpp"
#include "define.h"

#include <csignal>

int main() {
    signal(SIGPIPE, SIG_IGN);
    server_t _s;
    _s._tcp_backlog = DEFAULT_LISTEN_BACKLOG;
    _s._max_client_count = DEFAULT_MAX_CLIENT_COUNT;
    _s._port = DEFAULT_LISTEN_PORT;
    _s.init();
    _s.wait();
}