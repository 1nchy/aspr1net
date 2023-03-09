#include "ae.hpp"
#include "ae_epoll.hpp"

#include <cstdio>
#include <sys/time.h>

#include <poll.h>
#include <cerrno>

void _S_current_time(long& _s, long& _ms) {
    struct timeval _tv;
    gettimeofday(&_tv, nullptr);
    _s = _tv.tv_sec;
    _ms = _tv.tv_usec / 1000;
}
// char* ae_api_name() { return _S_ae_api_name(); }

ae_file_event::ae_file_event(int _fd, int _mask, proc_type* _proc, void* _data) {
    this->_mask |= _mask;
    if (_mask & AE_READABLE) {
        this->_r_proc = _proc;
    }
    if (_mask & AE_WRITABLE) {
        this->_w_proc = _proc;
    }
    this->_client_data = _data;
};

ae_time_event::ae_time_event(long long _id, long long _ms, proc_type* _proc, void* _data, timeup_proc_type* _t_proc) {
    this->_id = _id;
    long _cur_s, _cur_ms;
    _S_current_time(_cur_s, _cur_ms);
    this->_when_sec = _cur_s + _ms / 1000;
    this->_when_ms = _cur_ms + _ms % 1000;
    while (_when_ms >= 1000) {
        ++_when_sec;
        _when_ms -= 1000;
    }
    this->_proc = _proc;
    this->_timeup_proc = _t_proc;
    this->_client_data = _data;
}

ae_event_loop::ae_event_loop(int _size) {
    this->_events = new ae_file_event[_size];
    this->_fired = new ae_fired_event[_size];
    this->_fdset_size = _size;
    this->_last_time = time(nullptr);
    this->_time_event_list.clear();
    this->_time_event_next_id = 0;
    this->_stop = 0;
    this->_max_fd = -1;
    this->_bs_proc = nullptr;

    // todo aeApiCreate done
    this->_api_data = ae_epoll_state::_S_epoll_create(_size);

    for (int i = 0;  i < _size; ++i) {
        this->_events[i]._mask = AE_NONE;
    }
}
ae_event_loop::~ae_event_loop() {
    // todo aeApiFree done
    delete[] _events;
    delete[] _fired;
    _time_event_list.clear();
}
void ae_event_loop::stop() {
    this->_stop = 1;
}
void ae_event_loop::run() {
    this->_stop = 0;
    while (!this->_stop) {
        if (this->_bs_proc) {
            this->_bs_proc(this);
        }
        process_events(AE_ALL_EVENTS);
    }
}
int ae_event_loop::add_file_event(int _fd, int _mask, ae_file_event::proc_type* _proc, void* _data) {
    if (_fd >= this->_fdset_size) { return AE_ERR; }
    ae_epoll_state* _s = static_cast<ae_epoll_state*>(this->_api_data);
    ae_file_event* _fe = &this->_events[_fd];
    if (_s == nullptr) {
        return AE_ERR;
    }
    if (_fe->_mask == AE_NONE) {
        if (_s->add_event(_fd, _mask | _fe->_mask) == -1) return AE_ERR;
    }
    else {
        _s->mod_event(_fd, _mask | _fe->_mask);
    }
    _fe->_mask |= _mask;
    if (_mask & AE_READABLE) {
        _fe->_r_proc = _proc;
    }
    if (_mask & AE_WRITABLE) {
        _fe->_w_proc = _proc;
    }
    _fe->_client_data = _data;
    this->_max_fd = std::max(_max_fd, _fd);
    return AE_OK;
}
void ae_event_loop::del_file_event(int _fd, int _mask) {
    if (_fd >= this->_fdset_size) { return; }
    ae_epoll_state* _s = static_cast<ae_epoll_state*>(this->_api_data);
    ae_file_event* _fe = &this->_events[_fd];
    if (_s == nullptr) { return; }
    _mask = _fe->_mask & (~_mask);
    if (_mask == AE_NONE) {
        _s->del_event(_fd, _mask);
    }
    else {
        _s->mod_event(_fd, _mask);
    }
    _fe->_mask = _mask;
    if (_fd == _max_fd && _fe->_mask == AE_NONE) {
        int i;
        for (i = _max_fd - 1; i >= 0; --i) {
            if (_events[i]._mask != AE_NONE) {
                break;
            }
        }
        this->_max_fd = i;
    }
}
int ae_event_loop::get_file_event(int _fd) {
    if (_fd >= this->_fdset_size) { return 0; }
    return this->_events[_fd]._mask;
}
long long ae_event_loop::add_time_event(long long _ms, ae_time_event::proc_type* _proc, void* _data, ae_time_event::timeup_proc_type* _t_proc) {
    ae_time_event* _te = new ae_time_event(_time_event_next_id++, _ms, _proc, _data, _t_proc);
    _time_event_list.push_front(_te);
    return _time_event_list.front()->_id;
}
int ae_event_loop::del_time_event(long long _id) {
    for (auto i = _time_event_list.begin(); i != _time_event_list.end(); ++i) {
        if ((*i)->_id == _id) {
            (*i)->_id = AE_DELETED_EVENT_ID;
            return AE_OK;
        }
    }
    return AE_ERR;
}
int ae_event_loop::fdset_size() const {
    return _fdset_size;
}
int ae_event_loop::resize(int _new_size) {
    if (_new_size == fdset_size()) { return AE_OK; }
    if (_max_fd >= _new_size) { return AE_ERR; }
    ae_epoll_state* _s = static_cast<ae_epoll_state*>(this->_api_data);
    _s->resize(_new_size);
    ae_file_event* _old_events = _events;
    _events = new ae_file_event[_new_size];
    std::move(_old_events, _old_events + std::min(_fdset_size, _new_size), _events);
    delete[] _old_events;
    ae_fired_event* _old_fired = _fired;
    _fired = new ae_fired_event[_new_size];
    std::move(_old_fired, _old_fired + std::min(_fdset_size, _new_size), _fired);
    delete[] _old_events;
    _fdset_size = _new_size;

    for (int i = _max_fd + 1; i < _new_size; ++i) {
        this->_events[i]._mask = AE_NONE;
    }
    return AE_OK;
}
int ae_event_loop::process_events(int _flags) {
    if (!(_flags & AE_TIME_EVENTS) && !(_flags & AE_FILE_EVENTS)) {
        return 0;
    }

    int _events_num = 0;
    int _processed = 0;
    ae_epoll_state* _s = static_cast<ae_epoll_state*>(this->_api_data);
    // if (file_event || non-wait time event)
    if (this->_max_fd != -1 || ((_flags & AE_TIME_EVENTS) && !(_flags & AE_DONT_WAIT))) {
        ae_time_event* _shortest = nullptr;
        struct timeval _tv, *_tvp = nullptr;
        if ((_flags & AE_TIME_EVENTS) && !(_flags & AE_DONT_WAIT)) {
            _shortest = nearest_time_event();
        }
        if (_shortest != nullptr) {
            long _ns, _nms;
            _S_current_time(_ns, _nms);
            long long _ms = (_shortest->_when_sec - _ns) * 1000 + (_shortest->_when_ms - _nms);
            if (_ms > 0) {
                _tv.tv_sec = _ms / 1000;
                _tv.tv_usec = (_ms % 1000) * 1000;
            }
            else {
                _tv.tv_sec = _tv.tv_usec = 0;
            }
            _tvp = &_tv;
        }
        else {
            if (_flags & AE_DONT_WAIT) {
                _tv.tv_sec = _tv.tv_usec = 0;
                _tvp = &_tv;
            }
            else {
                _tvp = nullptr;
            }
        }
        _events_num = this->poll(_tvp);
        for (int i = 0; i < _events_num; ++i) {
            ae_file_event *_fe = &_events[_fired[i]._fd];
            int _mask = _fired[i]._mask;
            int _fd = _fired[i]._fd;

            bool _rfired = false;
            if (_fe->_mask & _mask & AE_READABLE) {
                _rfired = true;
                _fe->_r_proc(this, _fd, _fe->_client_data, _mask);
            }
            if (_fe->_mask & _mask & AE_WRITABLE) {
                if (!_rfired || _fe->_r_proc != _fe->_w_proc) {
                    _fe->_w_proc(this, _fd, _fe->_client_data, _mask);
                }
            }
            ++_processed;
        }
    }
    if (_flags & AE_TIME_EVENTS) {
        _processed += process_time_events();
    }
    return _processed;
}
int ae_event_loop::process_time_events() {
    return 0; // todo
}
void ae_event_loop::set_bs_proc(before_sleep_proc_type* _proc) {
    this->_bs_proc = _proc;
}

// protected implement:

ae_time_event* ae_event_loop::nearest_time_event() const {
    ae_time_event* _ret = nullptr;
    for (auto i = _time_event_list.begin(); i != _time_event_list.end(); ++i) {
        if (_ret == nullptr ||
            (*i)->_when_sec < _ret->_when_sec ||
            ((*i)->_when_sec == _ret->_when_sec && (*i)->_when_ms < _ret->_when_ms)) {
            _ret = *i;
        }
    }
    return _ret;
}
int ae_event_loop::poll(struct timeval* _tvp) {
    ae_epoll_state* _s = static_cast<ae_epoll_state*>(_api_data);
    int _events_num = _s->wait(_tvp);
    if (_events_num > 0) {
        for (int i = 0; i < _events_num; ++i) {
            struct epoll_event* _e = _s->_events + i;
            int _mask = 0;
            if (_e->events & EPOLLIN) {
                _mask |= AE_READABLE;
            }
            if (_e->events & EPOLLOUT) {
                _mask |= AE_WRITABLE;
            }
            if (_e->events & EPOLLERR) {
                _mask |= AE_WRITABLE;
            }
            if (_e->events & EPOLLHUP) {
                _mask |= AE_WRITABLE;
            }
            _fired[i]._fd = _e->data.fd;
            _fired[i]._mask = _mask;
        }
    }
    return _events_num;
}


// static void aeGetTime(long *seconds, long *milliseconds) {
//     struct timeval tv;

//     gettimeofday(&tv, NULL);
//     *seconds = tv.tv_sec;
//     *milliseconds = tv.tv_usec / 1000;
// }

// static void aeAddMillisecondsToNow(long long milliseconds, long *sec, long *ms) {
//     long cur_sec, cur_ms, when_sec, when_ms;

//     aeGetTime(&cur_sec, &cur_ms);
//     when_sec = cur_sec + milliseconds / 1000;
//     when_ms = cur_ms + milliseconds % 1000;
//     if (when_ms >= 1000) {
//         when_sec++;
//         when_ms -= 1000;
//     }
//     *sec = when_sec;
//     *ms = when_ms;
// }

// long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
//                             aeTimeProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc) {
//     long long id = eventLoop->timeEventNextId++;
//     aeTimeEvent *te;

//     te = zmalloc(sizeof(*te));
//     if (te == NULL) {
//         return AE_ERR;
//     }

//     te->id = id;
//     aeAddMillisecondsToNow(milliseconds, &te->when_sec, &te->when_ms);
//     te->timeProc = proc;
//     te->finalizerProc = finalizerProc;
//     te->clientData = clientData;

//     te->next = eventLoop->timeEventHead;
//     eventLoop->timeEventHead = te;

//     return id;
// }

// int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id) {
//     aeTimeEvent *te = eventLoop->timeEventHead;
//     while (te) {
//         if (te->id == id) {
//             te->id = AE_DELETED_EVENT_ID;
//             return AE_OK;
//         }
//         te = te->next;
//     }

//     return AE_ERR; /* NO event with the specified ID found */
// }

/* Search the first timer to fire.
 * This operation is useful to know how many time the select can be
 * put in sleep without to delay any event.
 * If there are no timers NULL is returned.
 *
 * Note that's O(N) since time events are unsorted.
 * Possible optimizations (not needed by Redis so far, but...):
 * 1) Insert the event in order, so that the nearest is just the head.
 *    Much better but still insertion or deletion of timers is O(N).
 * 2) Use a skiplist to have this operation as O(1) and insertion as O(log(N)).
 */
// static aeTimeEvent *aeSearchNearestTimer(aeEventLoop *eventLoop) {
//     aeTimeEvent *te = eventLoop->timeEventHead;
//     aeTimeEvent *nearest = NULL;

//     while (te) {
//         if (!nearest || te->when_sec < nearest->when_sec ||
//             (te->when_sec == nearest->when_sec && te->when_ms < nearest->when_ms)) {
//             nearest = te;
//         }
//         te = te->next;
//     }

//     return nearest;
// }

/* Process time events */
// static int processTimeEvents(aeEventLoop *eventLoop) {
//     int processed = 0;
//     aeTimeEvent *te, *prev;
//     long long maxId;
//     time_t now = time(NULL);

//     /* If the system clock is moved to the future, and then set back to the
//      * right value, time events may be delayed in a random way. Often this
//      * means that scheduled operations will not be performed soon enough.
//      *
//      * Here we try to detect system clock skews, and force all the time
//      * events to be processed ASAP when this happens: the idea is that
//      * processing events earlier is less dangerous than delaying them
//      * indefinitely, and practice suggests it is. */
//     if (now < eventLoop->lastTime) {
//         te = eventLoop->timeEventHead;
//         while (te) {
//             te->when_sec = 0;
//             te = te->next;
//         }
//     }
//     eventLoop->lastTime = now;

//     prev = NULL;
//     te = eventLoop->timeEventHead;
//     maxId = eventLoop->timeEventNextId - 1;
//     while (te) {
//         long now_sec, now_ms;
//         long long id;

//         /* Remove events scheduled for deletion. */
//         if (te->id == AE_DELETED_EVENT_ID) {
//             aeTimeEvent *next = te->next;
//             if (prev == NULL) {
//                 eventLoop->timeEventHead = te->next;
//             } else {
//                 prev->next = te->next;
//             }

//             if (te->finalizerProc) {
//                 te->finalizerProc(eventLoop, te->clientData);
//             }

//             zfree(te);
//             te = next;
//             continue;
//         }

//         /* Make sure we don't process time events created by time events in
//          * this iteration. Note that this check is currently useless: we always
//          * add new timers on the head, however if we change the implementation
//          * detail, this check may be useful again: we keep it here for future
//          * defense. */
//         if (te->id > maxId) {
//             te = te->next;
//             continue;
//         }
//         aeGetTime(&now_sec, &now_ms);
//         if (now_sec > te->when_sec || (now_sec == te->when_sec && now_ms >= te->when_ms)) {
//             int retval;

//             id = te->id;
//             retval = te->timeProc(eventLoop, id, te->clientData);
//             processed++;
//             if (retval != AE_NOMORE) {
//                 aeAddMillisecondsToNow(retval, &te->when_sec, &te->when_ms);
//             } else {
//                 te->id = AE_DELETED_EVENT_ID;
//             }
//         }
//         prev = te;
//         te = te->next;
//     }

//     return processed;
// }

/* Process every pending time event, then every pending file event
 * (that may be registered by time event callbacks just processed).
 * Without special flags the function sleeps until some file event
 * fires, or when the next time event occurs (if any).
 *
 * If flags is 0, the function does nothing and returns.
 * if flags has AE_ALL_EVENTS set, all the kind of events are processed.
 * if flags has AE_FILE_EVENTS set, file events are processed.
 * if flags has AE_TIME_EVENTS set, time events are processed.
 * if flags has AE_DONT_WAIT set the function returns ASAP until all
 * the events that's possible to process without to wait are processed.
 *
 * The function returns the number of events processed. */
// int aeProcessEvents(aeEventLoop *eventLoop, int flags) {
//     int processed = 0, numevents;

//     /* Nothing to do? return ASAP */
//     if (!(flags & AE_TIME_EVENTS) && !(flags & AE_FILE_EVENTS)) {
//         return 0;
//     }

//     /* Note that we want call select() even if there are no
//      * file events to process as long as we want to process time
//      * events, in order to sleep until the next time event is ready
//      * to fire. */
//     if (eventLoop->maxfd != -1 || ((flags & AE_TIME_EVENTS) && !(flags & AE_DONT_WAIT))) {
//         aeTimeEvent *shortest = NULL;
//         struct timeval tv, *tvp;

//         if (flags & AE_TIME_EVENTS && !(flags & AE_DONT_WAIT)) {
//             shortest = aeSearchNearestTimer(eventLoop);
//         }

//         if (shortest) {
//             long now_sec, now_ms;
//             aeGetTime(&now_sec, &now_ms);
//             tvp = &tv;

//             /* How many milliseconds we need to wait for the next time event to fire? */
//             long long ms = (shortest->when_sec - now_sec) * 1000 + shortest->when_ms - now_ms;

//             if (ms > 0) {
//                 tvp->tv_sec = ms / 1000;
//                 tvp->tv_usec = (ms % 1000) * 1000;
//             } else {
//                 tvp->tv_sec = 0;
//                 tvp->tv_usec = 0;
//             }
//         } else {
//             /* If we have to check for events but need to return
//              * ASAP because of AE_DONT_WAIT we need to set the timeout to zero */
//             if (flags & AE_DONT_WAIT) {
//                 tv.tv_sec = tv.tv_usec = 0;
//                 tvp = &tv;
//             } else {
//                 /* Otherwise we can block */
//                 tvp = NULL; /* wait forever */
//             }
//         }

//         numevents = aeApiPoll(eventLoop, tvp);
//         for (int i = 0; i < numevents; i++) {
//             aeFileEvent *fe = &eventLoop->events[eventLoop->fired[i].fd];
//             int mask = eventLoop->fired[i].mask;
//             int fd = eventLoop->fired[i].fd;

//             int rfired = 0;
//             /* note the fe->mask & mask & ... code: maybe an already processed
//              * event removed an element that fired and we still didn't
//              * processed, so we check if the event is still valid. */
//             if (fe->mask & mask & AE_READABLE) {
//                 rfired = 1;
//                 fe->rfileProc(eventLoop, fd, fe->clientData, mask);
//             }
//             if (fe->mask & mask & AE_WRITABLE) {
//                 if (!rfired || fe->wfileProc != fe->rfileProc) {
//                     fe->wfileProc(eventLoop, fd, fe->clientData, mask);
//                 }
//             }
//             processed++;
//         }
//     }
//     /* Check time events */
//     if (flags & AE_TIME_EVENTS) {
//         processed += processTimeEvents(eventLoop);
//     }

//     return processed; /* return the number of processed file/time events */
// }

/* Wait for milliseconds until the given file descriptor becomes writable/readable/exception */
// int aeWait(int fd, int mask, long long milliseconds) {
//     struct pollfd pfd;
//     int retmask = 0, retval;

//     bzero(&pfd, sizeof(pfd));
//     pfd.fd = fd;

//     if (mask & AE_READABLE) {
//         pfd.events |= POLLIN;
//     }

//     if (mask & AE_WRITABLE) {
//         pfd.events |= POLLOUT;
//     }

//     if ((retval = poll(&pfd, 1, (int) milliseconds)) == 1) {
//         if (pfd.revents & POLLIN) {
//             retmask |= AE_READABLE;
//         }
//         if (pfd.revents & POLLOUT) {
//             retmask |= AE_WRITABLE;
//         }
//         if (pfd.revents & POLLERR) {
//             retmask |= AE_WRITABLE;
//         }
//         if (pfd.revents & POLLHUP) {
//             retmask |= AE_WRITABLE;
//         }
//         return retmask;
//     }

//     return retval;
// }

// void aeMain(aeEventLoop *eventLoop) {
//     eventLoop->stop = 0;
//     while (!eventLoop->stop) {
//         if (eventLoop->beforesleep) {
//             eventLoop->beforesleep(eventLoop);
//         }
//         aeProcessEvents(eventLoop, AE_ALL_EVENTS);
//     }
// }

// char *aeGetApiName(void) {
//     return aeApiName();
// }

// void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep) {
//     eventLoop->beforesleep = beforesleep;
// }
