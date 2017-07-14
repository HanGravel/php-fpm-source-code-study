
	/* $Id: fpm_events.h,v 1.9 2008/05/24 17:38:47 anight Exp $ */
	/* (c) 2007,2008 Andrei Nigmatulin */

#ifndef FPM_EVENTS_H
#define FPM_EVENTS_H 1

#define FPM_EV_TIMEOUT  (1 << 0)
#define FPM_EV_READ     (1 << 1)
#define FPM_EV_PERSIST  (1 << 2)
#define FPM_EV_EDGE     (1 << 3)

#define fpm_event_set_timer(ev, flags, cb, arg) fpm_event_set((ev), -1, (flags), (cb), (arg))

struct fpm_event_s {
	int fd;                   /* not set with FPM_EV_TIMEOUT */
	struct timeval timeout;   /* next time to trigger */
	struct timeval frequency;
	void (*callback)(struct fpm_event_s *, short, void *);
	void *arg;
	int flags;
	int index;                /* index of the fd in the ufds array */
	short which;              /* type of event */
};
/* FPM事件结构
 * int fd 文件描述符
 * struct timeval结构是linux自带结构
 *      struct timeval {
 *          time_t tv_sec; //seconds
 *          suseconds_t tv_usec; //microseconds
 *      };
 * void (*callback)(struct fpm_event_s *, short, void *);是该事件的回调函数
 * int index 是这个fd在ufds数组中的index
 * short which 是这个事件的类型
 */

typedef struct fpm_event_queue_s {
	struct fpm_event_queue_s *prev;
	struct fpm_event_queue_s *next;
	struct fpm_event_s *ev;
} fpm_event_queue;
/* FPM事件队列节点结构
 * 每个节点包含一个事件结构(struct fpm_event_s)
 * 每个节点指向prev节点和next节点，故为双向链表
 */

struct fpm_event_module_s {
	const char *name;
	int support_edge_trigger;
	int (*init)(int max_fd);
	int (*clean)(void);
	int (*wait)(struct fpm_event_queue_s *queue, unsigned long int timeout);
	int (*add)(struct fpm_event_s *ev);
	int (*remove)(struct fpm_event_s *ev);
};
/* FPM事件模块结构
 * const char *name; 该模块使用的io复用机制名称
 * int support_edge_trigger; 是否支持边缘触发模式
 * 该结构支持的函数 init clean wait add remove
 * 
 * 该结构由fpm_event_xxx_module()系列函数返回，
 * 每个函数具体实现由模块类型决定，定义在相应的模块文件中，如epoll.c,select.c等
 */


void fpm_event_loop(int err);
void fpm_event_fire(struct fpm_event_s *ev);
int fpm_event_init_main();
int fpm_event_set(struct fpm_event_s *ev, int fd, int flags, void (*callback)(struct fpm_event_s *, short, void *), void *arg);
int fpm_event_add(struct fpm_event_s *ev, unsigned long int timeout);
int fpm_event_del(struct fpm_event_s *ev);
int fpm_event_pre_init(char *machanism);
const char *fpm_event_machanism_name();
int fpm_event_support_edge_trigger();

#endif
