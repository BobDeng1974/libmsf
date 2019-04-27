/**************************************************************************
*
* Copyright (c) 2017-2019, luotang.me <wypx520@gmail.com>, China.
* All rights reserved.
*
* Distributed under the terms of the GNU General Public License v2.
*
* This software is provided 'as is' with no explicit or implied warranties
* in respect of its properties, including, but not limited to, correctness
* and/or fitness for purpose.
*
**************************************************************************/

/*
ʵ���ϣ���������Ľ���벻��Nginx��post�¼�������ơ����post�¼���ʲô��˼�أ�����ʾ�����¼��Ӻ�ִ�С�Nginx���������post���У�һ
�����ɱ������ļ������ӵĶ��¼����ɵ�ngx_posted_accept_events���У���һ��������ͨ����д�¼����ɵ�ngx_posted_events���С�������post��
���������û����ʲô���Ĺ����أ�
��epoll_wait������һ���¼����ֵ�������������,�ô�����������¼���ngx_posted_accept_events��������ִ�У������ͨ�¼���ngx_posted_events��
�����ִ�У����ǽ������Ⱥ���͸��ؾ�����������Ĺؼ�.
����ڴ���һ���¼��Ĺ����в�������һ���¼�,������ϣ������¼����ִ�У���������ִ�У���
�Ϳ��԰����ŵ�post�����С�
*/

#include <msf_time.h>
#include <msf_list.h>
#include <msf_network.h>

#define EPOLLEVENTS 100
#define LIMIT_TIMER 1 /* ���޴�����ʱ�� */
#define CYCLE_TIMER 2 /* ѭ����ʱ�� */

enum msf_event_flags {
    MSF_EVENT_TIMEOUT  = 1<<0,
    MSF_EVENT_READ     = 1<<1,
    MSF_EVENT_WRITE    = 1<<2,
    MSF_EVENT_SIGNAL   = 1<<3,
    MSF_EVENT_PERSIST  = 1<<4,
    MSF_EVENT_ET       = 1<<5,
    MSF_EVENT_FINALIZE = 1<<6,
    MSF_EVENT_CLOSED   = 1<<7,
    MSF_EVENT_ERROR    = 1<<8,
    MSF_EVENT_EXCEPT   = 1<<9,
}__attribute__((__packed__));

struct msf_event_cbs {
    void (*read_cbs)(void *arg);
    void (*write_cbs)(void *arg);
    void (*error_cbs)(void *arg);
    s32 (*timer_cbs)(void *arg);
    void *args;
}__attribute__((__packed__));

struct msf_event {
    /* for managing timeouts */
    union {
        struct list_head ev_list; /* ev_next_with_common_timeout */
        s32 min_heap_idx;
    } ev_timeout_pos;

    u32 ev_priority;
    u32 timer_id;
    struct timeval ev_interval;
    struct timeval ev_timeout;
    s32 ev_exe_num;

    struct msf_event_cbs *ev_cbs;

    s32 ev_fd;
    s32 ev_res; /* result passed to event callback */
    s32 ev_flags;
} __attribute__((__packed__));

struct msf_event_base;
struct msf_event_ops {
    void *(*init)();
    void (*deinit)(void *ctx);
    s32 (*add)(struct msf_event_base *eb, struct msf_event *ev);
    s32 (*del)(struct msf_event_base *eb, struct msf_event *ev);
    s32 (*dispatch)(struct msf_event_base *eb, struct timeval *tv);
}__attribute__((__packed__));

struct msf_event_base {
    /** Pointer to backend-specific data. */
    void *ctx;
    s32 loop;
    s32 rfd;
    s32 wfd;
    const struct msf_event_ops *ev_ops;
} __attribute__((__packed__));


s32 msf_timer_init(void);
void msf_timer_destroy(void);
s32 msf_timer_add(u32 timer_id, s32 interval, s32 (*fun)(void*), void *arg, s32 flag, s32 exe_num);
s32 msf_timer_remove(u32 timer_id);

struct msf_event *msf_event_new(s32 fd,
        void (*read_cbs)(void *),
        void (*write_cbs)(void *),
        void (*error_cbs)(void *),
        void *args);
void msf_event_free(struct msf_event *ev);
s32 msf_event_add(struct msf_event_base *eb, struct msf_event *ev);
s32 msf_event_del(struct msf_event_base *eb, struct msf_event *ev);

struct msf_event_base *msf_event_base_new(void);
void msf_event_base_free(struct msf_event_base *eb);
s32 msf_event_base_dispatch(struct msf_event_base *eb);
void msf_event_base_loopexit(struct msf_event_base *eb);
s32 msf_event_base_wait(struct msf_event_base *eb);
void msf_event_base_signal(struct msf_event_base *eb);

