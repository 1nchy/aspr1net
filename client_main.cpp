#include <cstdio>

#include "client.hpp"

#include "define.h"
#include "utils/log_utils.hpp"

int fgetline(char* _line, int _n, FILE* _stream) {
    int _len = 0; int _c;
    --_n;
    while ((_c = fgetc(_stream)) != EOF && _len < _n) {
        if (_c == '\n') {
            break;
        }
        _line[_len++] = _c;
    }
    if (_len == 0 && _c == EOF) {
        return -1;
    }
    _line[_len] = '\0';
    return _len;
}

int main(int argc, char* argv[]) {
    client_t _c;
    ASP_LOG("[client]");
    int _client_connected = -1;
    if (argc == 2) {
        const char* _ip = argv[1];
        _client_connected = _c.connect(_ip, DEFAULT_LISTEN_PORT);
        if (_client_connected == -1) {
            ASP_LOG("failed to connect.");
        }
    }
    char _command_str[128];
    while (_client_connected == -1) {
        printf("connect: ");
        if (fgetline(_command_str, 128, stdin) == -1) {
            return 0;
        }
        _client_connected = _c.connect(_command_str, DEFAULT_LISTEN_PORT);
        if (_client_connected == -1) {
            ASP_LOG("failed to connect.");
        }
    }
    while (1) {
        printf("> ");
        if (fgetline(_command_str, 128, stdin) == -1) {
            break;
        }
        _c.process(_command_str);
    }
    // _c.send("ping wang\\ yin\\ qi");
    // _c.write("hello from outside.");
    // _c.recv();
    // _c.file_recv("client-abc.txt");
    // _c.file_send("client-wyq.md");
    return 0;
}