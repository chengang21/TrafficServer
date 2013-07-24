//types.h

#ifndef _PROXY_TYPES_H_
#define _PROXY_TYPES_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "clusterinterface.h"
#include "libts.h"

//#define USE_MULTI_ALLOCATOR  1
#define CHECK_MAGIC_NUMBER  1

#define PRIORITY_COUNT      3   //priority queue count

//statistic marco defines
//#define TRIGGER_STAT_FLAG  1  //trigger statistic flag
//#define MSG_TIME_STAT_FLAG 1  //data statistic flag

#define MSG_HEADER_LENGTH   ((int)sizeof(MsgHeader))
#define MAGIC_NUMBER        0x3308
#define MAX_MSG_LENGTH      (4 * 1024 * 1024)

#define MAX_MACHINE_COUNT        255   //IMPORTANT: can't be 256!!

#define READ_BUFFER_SIZE    (2 * 1024 * 1024)

#define MAX_SESSION_COUNT_PER_MACHINE   1000000
#define SESSION_LOCK_COUNT_PER_MACHINE    10949

//combine multi msg to call writev
#define WRITEV_ARRAY_SIZE   256
#define WRITEV_ITEM_ONCE    (WRITEV_ARRAY_SIZE / 2)
#define WRITE_MAX_COMBINE_BYTES  (128 * 1024)

#define CONNECT_TYPE_CLIENT  'C'  //connect by me, client
#define CONNECT_TYPE_SERVER  'S'  //connect by peer, server

#define DATA_TYPE_BUFFER     'B'  //char buffer
#define DATA_TYPE_OBJECT     'O'  //IOBufferBlock pointer

#define ALIGN_BYTES  16
#define BYTE_ALIGN(x,l)  (((x) + ((l) - 1)) & ~((l) - 1))
#define BYTE_ALIGN16(x)  BYTE_ALIGN(x, ALIGN_BYTES)

#define IS_SESSION_EMPTY(session_id) \
  ((session_id).ids[0] == 0 && (session_id).ids[1] == 0)

#define IS_SESSION_EQUAL(session_id1, session_id2) \
  ((session_id1).ids[0] == (session_id2).ids[0] && \
   (session_id1).ids[1] == (session_id2).ids[1])

typedef struct msg_timeused {
  volatile int64_t count;     //message count
  volatile int64_t time_used; //time used
} MsgTimeUsed;

typedef struct session_stat {
  volatile int64_t create_total_count;   //create session total count
  volatile int64_t create_success_count; //create session success count
  volatile int64_t create_retry_times;   //create session retry times
  volatile int64_t close_total_count;    //close session count
  volatile int64_t close_success_count;  //close session success count
  volatile int64_t session_miss_count;     //session miss count
  volatile int64_t session_occupied_count; //session occupied count
} SessionStat;

typedef struct msg_header {
#ifdef CHECK_MAGIC_NUMBER
  short magic;            //magic number
  unsigned short msg_seq; //message sequence no base 1
#else
  uint32_t msg_seq; //message sequence no base 1
#endif

	int func_id; //function id, must be signed int
	int data_len; //message body length
  int aligned_data_len;  //aligned body length
	SessionId session_id; //session id
} MsgHeader;   //must aligned by 16 bytes

typedef struct in_msg_entry {
	int func_id;  //function id
	int data_len; //message body length
  Ptr<IOBufferBlock> blocks;
	struct in_msg_entry *next; //for income message queue
} InMessage;

typedef struct session_entry {
	SessionId session_id;
	void *user_data;  //user data for callback
	struct socket_context *sock_context;
  InMessage *messages;  //income messages
  int response_events;  //response events
  uint32_t current_msg_seq;  //current message sequence no
  struct session_entry *next;  //session chain, only for server session

#ifdef TRIGGER_STAT_FLAG
  volatile int64_t stat_start_time;   //for message time used stat
#endif

#ifdef MSG_TIME_STAT_FLAG
  volatile int64_t client_start_time;  //send start time for client
  volatile int64_t server_start_time;  //recv done time for server
  volatile int64_t send_start_time; //send start time for stat send time
#endif

} SessionEntry;

typedef struct out_msg_entry {
  MsgHeader header;
  char mini_buff[MINI_MESSAGE_SIZE];  //for mini message
  Ptr<IOBufferBlock> blocks;

	struct out_msg_entry *next; //for send queue
	int bytes_sent;    //important: including msg header
  int data_type;     //buffer or object
} OutMessage;

typedef struct read_manager {
  Ptr<IOBufferData> buffer; //recv buffer
  Ptr<IOBufferBlock> blocks;    //recv blocks
  char *msg_header;  //current message start
  char *current;    //current pointer
  char *buff_end;   //buffer end
  int recv_body_bytes;  //recveived body bytes
} ReaderManager;

struct worker_thread_context;

typedef struct {
  OutMessage *head;
  OutMessage *tail;
  pthread_mutex_t lock;
} MessageQueue;

typedef struct socket_context {
	int sock;  //socket fd
  char padding[ALIGN_BYTES]; //padding buffer
	struct read_manager reader;  //recv buffer
  struct ClusterMachine *machine;      //peer machine, point to global machine
	struct worker_thread_context *thread_context;
  MessageQueue send_queues[PRIORITY_COUNT];  //queue for send
  pthread_mutex_t lock;

  int queue_index;  //current deal queue index
  int connect_type;       //client or server
  time_t connected_time;  //connection established timestamp
	uint32_t remain_events; //deal remain epoll event
	uint32_t epoll_events;  //current epoll event for trigger

#ifdef USE_MULTI_ALLOCATOR
  Allocator *out_msg_allocator;  //for send
  Allocator *in_msg_allocator;   //for notify dealer
#endif

  struct socket_context *nio_next;  //for nio schedule (read and write)
  struct socket_context *next;  //for freelist
} SocketContext;

struct worker_thread_context
{
	int epoll_fd;
	int alloc_size;         //for epoll events
	int thread_index;       //my thread index
  int __pad;              //pad field
	pthread_mutex_t lock;
	struct epoll_event *events;  //for epoll_wait
};

#endif

