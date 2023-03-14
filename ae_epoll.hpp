#ifndef _ASP_AE_EPOLL_HPP_
#define _ASP_AE_EPOLL_HPP_

#include <cstdlib>
#include <unistd.h>
#include <ctime>
#include <sys/epoll.h>

#include <memory>

#include "define.h"
#include "ae.hpp"

struct ae_epoll_state;

struct ae_epoll_state {
    int _epoll_fd;
    int _events_size = 0;
    struct epoll_event *_events = nullptr;

private:
    ae_epoll_state(int _size);
    virtual ~ae_epoll_state();
public:
    static ae_epoll_state* _S_epoll_create(int _size);
    static void _S_epoll_destroy(ae_epoll_state* _state);
    void resize(int _new_size);
    int add_event(int _fd, int _mask);
    void mod_event(int _fd, int _mask);
    void del_event(int _fd, int _mask);
    // todo aeApiPoll
    int wait(struct timeval *_tvp);
};

ae_epoll_state::ae_epoll_state(int _size) {
    this->_events = new epoll_event[_size];
    this->_events_size = _size;
}
ae_epoll_state::~ae_epoll_state() {
    delete[] this->_events;
    this->_events = nullptr;
    this->_events_size = 0;
}
void ae_epoll_state::resize(int _new_size) {
    struct epoll_event* _old_events = this->_events;
    int _old_events_size = this->_events_size;
    this->_events = new epoll_event[_new_size];
    std::move(_old_events, _old_events + std::min(_old_events_size, _new_size), this->_events);
    this->_events_size = _new_size;
    delete[] _old_events;
    // state->events = zrealloc(state->events, sizeof(struct epoll_event) * setsize);
}
int ae_epoll_state::add_event(int _fd, int _mask) {
    struct epoll_event _ee = {0};
    _ee.events = 0;
    if (_mask & AE_READABLE) {
        _ee.events |= EPOLLIN;
    }
    if (_mask & AE_WRITABLE) {
        _ee.events |= EPOLLOUT;
    }
    // _ee.events |= EPOLLET;
    _ee.data.fd = _fd;
    int _op = EPOLL_CTL_ADD;
    if (epoll_ctl(this->_epoll_fd, _op, _fd, &_ee) == -1) {
        return -1;
    }
    return 0;
}
void ae_epoll_state::mod_event(int _fd, int _mask) {
    struct epoll_event _ee = {0};
    _ee.events = 0;
    if (_mask & AE_READABLE) {
        _ee.events |= EPOLLIN;
    }
    if (_mask & AE_WRITABLE) {
        _ee.events |= EPOLLOUT;
    }
    _ee.data.fd = _fd;
    int _op = EPOLL_CTL_MOD;
    epoll_ctl(this->_epoll_fd, _op, _fd, &_ee) == -1;
}
void ae_epoll_state::del_event(int _fd, int _mask) {
    struct epoll_event _ee = {0};
    _ee.events = 0;
    if (_mask & AE_READABLE) {
        _ee.events |= EPOLLIN;
    }
    if (_mask & AE_WRITABLE) {
        _ee.events |= EPOLLOUT;
    }
    _ee.data.fd = _fd;
    int _op = EPOLL_CTL_DEL;
    epoll_ctl(this->_epoll_fd, _op, _fd, &_ee) == -1;
}
int ae_epoll_state::wait(struct timeval *_tvp) {
    int _timeout = _tvp ? (_tvp->tv_sec * 1000 + _tvp->tv_usec / 1000) : -1;
    return epoll_wait(_epoll_fd, _events, _events_size, _timeout);
    // todo aeApiPoll
}

ae_epoll_state* ae_epoll_state::_S_epoll_create(int _size) {
    ae_epoll_state *_ret = new ae_epoll_state(_size);

    _ret->_epoll_fd = epoll_create(1024);  /* 1024 is just a hint for the kernel */
    if (_ret->_epoll_fd == -1) {
        goto err;
    }
    return _ret;

err:
    delete _ret;
    return nullptr;
}

void ae_epoll_state::_S_epoll_destroy(ae_epoll_state *_state) {
    if (_state == nullptr) { return; }
    close(_state->_epoll_fd);
    _state->~ae_epoll_state();
}

// static int aeApiAddEvent(aeEventLoop *eventLoop, int fd, int mask) {
//     ae_epoll_state *state = static_cast<ae_epoll_state*>(eventLoop->apidata);
//     struct epoll_event ee = {0};    /* avoid valgrind warning */

//     /* If the fd was already monitored for some event, we need a MOD
//      * operation. Otherwise we need an ADD operation. */
//     int op = eventLoop->events[fd].mask == AE_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;

//     ee.events = 0;
//     mask |= eventLoop->events[fd].mask; /* Merge old events */
//     if (mask & AE_READABLE) {
//         ee.events |= EPOLLIN;
//     }
//     if (mask & AE_WRITABLE) {
//         ee.events |= EPOLLOUT;
//     }

//     ee.data.fd = fd;
//     if (epoll_ctl(state->epfd, op, fd, &ee) == -1) {
//         return -1;
//     }

//     return 0;
// }

// static void aeApiDelEvent(aeEventLoop *eventLoop, int fd, int delmask) {
//     ae_epoll_state *state = static_cast<ae_epoll_state*>(eventLoop->apidata);
//     struct epoll_event ee = {0}; /* avoid valgrind warning */
//     int mask = eventLoop->events[fd].mask & (~delmask);

//     ee.events = 0;
//     if (mask & AE_READABLE) {
//         ee.events |= EPOLLIN;
//     }

//     if (mask & AE_WRITABLE) {
//         ee.events |= EPOLLOUT;
//     }

//     ee.data.fd = fd;
//     if (mask != AE_NONE) {
//         epoll_ctl(state->epfd, EPOLL_CTL_MOD, fd, &ee);
//     } else {
//         /* Note, Kernel < 2.6.9 requires a non null event pointer even for EPOLL_CTL_DEL. */
//         epoll_ctl(state->epfd, EPOLL_CTL_DEL, fd, &ee);
//     }
// }

// static int aeApiPoll(aeEventLoop *eventLoop, struct timeval *tvp) {
//     ae_epoll_state *state = static_cast<ae_epoll_state*>(eventLoop->apidata);
//     int retval, numevents = 0;

//     retval = epoll_wait(state->epfd, state->events, eventLoop->setsize,
//                         tvp ? (tvp->tv_sec * 1000 + tvp->tv_usec / 1000) : -1);
//     if (retval > 0) {
//         numevents = retval;
//         for (int i = 0; i < numevents; i++) {
//             int mask = 0;
//             struct epoll_event *e = state->events + i;

//             if (e->events & EPOLLIN) {
//                 mask |= AE_READABLE;
//             }
//             if (e->events & EPOLLOUT) {
//                 mask |= AE_WRITABLE;
//             }
//             if (e->events & EPOLLERR) {
//                 mask |= AE_WRITABLE;
//             }
//             if (e->events & EPOLLHUP) {
//                 mask |= AE_WRITABLE;
//             }
//             eventLoop->fired[i].fd = e->data.fd;
//             eventLoop->fired[i].mask = mask;
//         }
//     }

//     return numevents;
// }

// static char *_S_ae_api_name(void) {
//     return "epoll";
// }

#endif // _ASP_AE_EPOLL_HPP_