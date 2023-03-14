#include "client.hpp"

#include "define.h"
#include "utils/log_utils.hpp"

int main(void) {
    client_t _c;
    _c.connect("127.0.0.1", DEFAULT_LISTEN_PORT);
    // _c.send("ping");
    // _c.write("hello from outside.");
    // _c.recv();
    // _c.file_recv("client-abc.txt");
    _c.file_send("client-wyq.md");
    return 0;
}