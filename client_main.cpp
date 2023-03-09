#include "client.hpp"

#include "define.h"
#include "utils/log_utils.hpp"

int main(void) {
    client_t _c;
    _c.connect("127.0.0.1", DEFAULT_LISTEN_PORT);
    _c.write("hello from outside.");
    _c.read();
    ASP_LOG("reply: %s\n", _c._r_buffer._buff);
    return 0;
}