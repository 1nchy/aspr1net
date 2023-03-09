#ifndef _ASP_AE_HPP_
#define _ASP_AE_HPP_

#include <ctime>
#include <list>

#define AE_OK        0
#define AE_ERR      -1

#define AE_NONE      0
#define AE_READABLE  1
#define AE_WRITABLE  2

#define AE_FILE_EVENTS  1
#define AE_TIME_EVENTS  2
#define AE_ALL_EVENTS    (AE_FILE_EVENTS|AE_TIME_EVENTS)
#define AE_DONT_WAIT    4

#define AE_NOMORE            -1
#define AE_DELETED_EVENT_ID  -1

/* Macros */
#define AE_NOTUSED(V) ((void) V)

// struct aeEventLoop;
struct ae_event_loop;
void _S_current_time(long& _s, long& _ms);
// char* ae_api_name();

/* Types and data structures */
// typedef void aeFileProc(struct aeEventLoop *eventLoop, int fd, void *clientData, int mask);
// typedef int aeTimeProc(struct aeEventLoop *eventLoop, long long id, void *clientData);
// typedef void aeEventFinalizerProc(struct aeEventLoop *eventLoop, void *clientData);
// typedef void aeBeforeSleepProc(struct aeEventLoop *eventLoop);

/* File event structure */
// typedef struct aeFileEvent {
//     int mask;       /* one of AE_(READABLE|WRITABLE) */
//     aeFileProc *rfileProc;
//     aeFileProc *wfileProc;
//     void *clientData;
// } aeFileEvent;

struct ae_file_event {
    typedef void proc_type(ae_event_loop*, int, void*, int);
    friend struct ae_event_loop;

    int _mask = 0;
    proc_type *_r_proc = nullptr;
    proc_type *_w_proc = nullptr;
    void* _client_data = nullptr;

private:
    ae_file_event() = default;
    ae_file_event(int, int, proc_type*, void*);
    virtual ~ae_file_event() = default;
};

/* Time event structure */
// typedef struct aeTimeEvent {
//     long long id;       /* time event identifier. */
//     long when_sec;      /* seconds */
//     long when_ms;       /* milliseconds */
//     aeTimeProc *timeProc;
//     aeEventFinalizerProc *finalizerProc;
//     void *clientData;
//     struct aeTimeEvent *next;
// } aeTimeEvent;

struct ae_time_event {
    typedef int proc_type(ae_event_loop*, long long, void*);
    typedef void timeup_proc_type(ae_event_loop*, void*);
    friend struct ae_event_loop;

    long long _id;
    long _when_sec;
    long _when_ms;
    proc_type *_proc = nullptr;
    timeup_proc_type *_timeup_proc = nullptr;
    void* _client_data = nullptr;

private:
    ae_time_event() = default;
    ae_time_event(long long, long long, proc_type*, void*, timeup_proc_type*);
    virtual ~ae_time_event() = default;
};

/* A fired event */
// typedef struct aeFiredEvent {
//     int fd;
//     int mask;
// } aeFiredEvent;

struct ae_fired_event {
    int _fd;
    int _mask;
};

/* State of an event based program */
// typedef struct aeEventLoop {
//     int maxfd;                  /* highest file descriptor currently registered */
//     int setsize;                /* max number of file descriptors tracked */
//     long long timeEventNextId;
//     time_t lastTime;            /* Used to detect system clock skew */
//     aeFileEvent *events;        /* Registered events */
//     aeFiredEvent *fired;        /* Fired events */
//     aeTimeEvent *timeEventHead;
//     int stop;
//     void *apidata;              /* This is used for polling API specific data */
//     aeBeforeSleepProc *beforesleep;
// } aeEventLoop;

struct ae_event_loop {
    typedef void before_sleep_proc_type(ae_event_loop*);

    int _max_fd;  // highest file descriptor currently registered
    int _fdset_size;  // max number of file descriptors tracked
    long long _time_event_next_id;
    time_t _last_time;
    ae_file_event *_events;        /* Registered events */
    ae_fired_event *_fired;        /* Fired events */
    std::list<ae_time_event*> _time_event_list;
    int _stop;
    void* _api_data;
    before_sleep_proc_type* _bs_proc;

public:
    ae_event_loop(int _size);
    virtual ~ae_event_loop();
    void stop();
    void run();

    int add_file_event(int, int, ae_file_event::proc_type*, void*);
    void del_file_event(int, int);
    int get_file_event(int);

    long long add_time_event(long long, ae_time_event::proc_type*, void*, ae_time_event::timeup_proc_type*);
    int del_time_event(long long);

    int fdset_size() const;
    int resize(int);

    int process_events(int);
    int process_time_events();

    void set_bs_proc(before_sleep_proc_type* _proc);

protected:
    ae_time_event* nearest_time_event() const;
    int poll(struct timeval*);
};

// aeEventLoop *aeCreateEventLoop(int setsize);

// void aeDeleteEventLoop(aeEventLoop *eventLoop);

// void aeStop(aeEventLoop *eventLoop);

// int aeCreateFileEvent(aeEventLoop *eventLoop, int fd, int mask, aeFileProc *proc, void *clientData);

// void aeDeleteFileEvent(aeEventLoop *eventLoop, int fd, int mask);

// int aeGetFileEvents(aeEventLoop *eventLoop, int fd);

// long long aeCreateTimeEvent(aeEventLoop *eventLoop, long long milliseconds,
//                             aeTimeProc *proc, void *clientData, aeEventFinalizerProc *finalizerProc);

// int aeDeleteTimeEvent(aeEventLoop *eventLoop, long long id);

// int aeProcessEvents(aeEventLoop *eventLoop, int flags);

// void aeMain(aeEventLoop *eventLoop);

// int aeWait(int fd, int mask, long long milliseconds);

// char *aeGetApiName(void);

// void aeSetBeforeSleepProc(aeEventLoop *eventLoop, aeBeforeSleepProc *beforesleep);

// int aeGetSetSize(aeEventLoop *eventLoop);

// int aeResizeSetSize(aeEventLoop *eventLoop, int setsize);

#endif //_ASP_AE_HPP_
