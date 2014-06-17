#ifndef _SOCKES_H
#define _SOCKES_H

#include "list.h"
#include "debug.h"
#include "socks5_protocol.h"
#include "buffer.h"
#include <stdint.h>
#include <stdlib.h>

#define AE_NONE 0
#define AE_READABLE 1
#define AE_WRITABLE 2

#define AE_NOMORE -1

struct ss_server_ctx;

struct conn_info {
	char ip[128];
	uint16_t port;
};

struct ss_fd_set {
	fd_set rfds, wfds;
	/* We need to have a copy of the fd sets as it's not safe to reuse
	 * FD sets after select(). */
	fd_set _rfds, _wfds;
};

typedef void ss_ioproc(void *owner, int fd, void *para, int mask);
typedef int ss_timeproc(struct ss_server_ctx *owner, long id, void *para);
typedef int callback_proc(void *data);

struct io_event {
	int mask; /* one of AE_(READABLE|WRITABLE) */
	ss_ioproc *rfileproc;
	ss_ioproc *wfileproc;
	void *para;
};

struct time_event {
	uint64_t id;	/* time event identifier. */
	long when_sec;	/* seconds */
	long when_ms;	/* milliseconds */
	ss_timeproc *timeproc;
	void *para;
	struct list_head list;
};

#define SS_SERVER_CTX 1
#define SS_CONN_CTX 2

struct fd_curr_state {
	int type;
	void *ctx_ptr;
};

struct ss_conn_ctx {
	int conn_fd;
	struct ss_server_ctx *server_entry;
	int fd_mask; /* one of AE_(READABLE|WRITABLE) */
	enum ss_state ss_conn_state;
	struct conn_info ss_conn_info;
	struct io_event io_proc;
	struct buf *msg;
	callback_proc *cb_proc;
	void *data;
	struct list_head list;
};

struct ss_server_ctx {
	int sock_fd;
	int fd_mask; /* one of AE_(READABLE|WRITABLE) */
	uint16_t s_port;
	uint32_t s_addr;
	int conn_count; /* 连接数 */
	struct ss_conn_ctx *conn;
	struct io_event io_proc;
	struct buf *buf;
	uint64_t time_event_next_id;
	struct time_event *time_event_list;
	callback_proc *g_cb_proc; /* 对每个连接都有效 */
	void *data;
	int max_fd;
	struct ss_fd_set *ss_allfd_set;
	struct fd_curr_state *fd_state;
};

struct ss_server_ctx *ss_create_server(uint16_t port);
void ss_release_server(struct ss_server_ctx *ss_server);
struct ss_conn_ctx *ss_server_add_conn(struct ss_server_ctx *s, int conn_fd,
		int mask, struct conn_info *conn_info, struct io_event *event);
void ss_server_del_conn(struct ss_server_ctx *s, struct ss_conn_ctx *conn);
int ss_handshake_handle(struct ss_conn_ctx *conn);
int ss_msg_handle(struct ss_conn_ctx *conn, 
		void (*func)(struct ss_conn_ctx *conn));
int ss_send_msg_conn(struct ss_conn_ctx *conn, int msg_type);
void ss_loop(struct ss_server_ctx *server);
int ss_server_add_time_event(struct ss_server_ctx *s, uint64_t ms, 
			ss_timeproc *proc, void *para);
void ss_server_del_time_event(struct time_event *te);

#endif
