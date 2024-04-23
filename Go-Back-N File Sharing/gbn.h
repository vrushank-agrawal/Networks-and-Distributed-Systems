#ifndef _gbn_h
#define _gbn_h

#include<sys/types.h>
#include<sys/socket.h>
#include<sys/ioctl.h>
#include <sys/time.h>
#include<signal.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<netinet/in.h>
#include<errno.h>
#include<netdb.h>
#include<time.h>

/*----- Error variables -----*/
extern int h_errno;
extern int errno;

/*----- Protocol parameters -----*/
#define LOSS_PROB 1e-2    /* loss probability                            */
#define CORR_PROB 1e-3    /* corruption probability                      */
#define DATALEN   1024    /* length of the payload                       */
#define N         1024    /* Max number of packets a single call to gbn_send can process */
#define TIMEOUT      1    /* timeout to resend packets (1 second)        */
#define MAX_ATTEMPTS 5    /* max number of attempts to send a frame     */
#define MAX_SEQNUM   256  /* max sequence number                         */

/*----- Packet types -----*/
#define SYN      0        /* Opens a connection                          */
#define SYNACK   1        /* Acknowledgement of the SYN frame           */
#define DATA     2        /* Data packets                                */
#define DATAACK  3        /* Acknowledgement of the DATA frame          */
#define FIN      4        /* Ends a connection                           */
#define FINACK   5        /* Acknowledgement of the FIN frame           */
#define RST      6        /* Reset frame used to reject new connections */

/*----- Go-Back-n frame format -----*/
typedef struct {
	uint8_t  type;            /* frame type (e.g. SYN, DATA, ACK, FIN)     */
	uint8_t  seqnum;          /* sequence number of the frame              */
    uint16_t checksum;        /* header and payload checksum                */
    uint8_t data[DATALEN];    /* pointer to the payload                     */
} __attribute__((packed)) gbnhdr;

typedef struct state_t{
	uint8_t curr_state;
	uint8_t seqnum;
	uint8_t window_size;
	struct sockaddr dest_addr;
	socklen_t dest_sock_len;
	int timeout;
} state_t;

typedef struct frame_t{
	char *frame_addr;
	gbnhdr *frame;
	u_int8_t seqnum;
	int len;
} frame_t;

enum {
	CLOSED=0,
	SYN_SENT,
	SYN_RCVD,
	ESTABLISHED,
	FIN_SENT,
	FIN_RCVD
};

extern state_t s;
extern struct itimerval itimer;
extern const char *states[];

/*----- Function prototypes -----*/
void gbn_init();
int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen);
int gbn_listen(int sockfd, int backlog);
int gbn_bind(int sockfd, const struct sockaddr *server, socklen_t socklen);
int gbn_socket(int domain, int type, int protocol);
int gbn_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int gbn_close(int sockfd);
ssize_t gbn_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags);
uint16_t checksum(gbnhdr *frame);

ssize_t  maybe_recvfrom(int s,
						char *buf,
						size_t len,
						int flags,
			            struct sockaddr *from,
						socklen_t *fromlen);

/*----- Auxiliary functions -----*/
void timeout_handler(int signal);
void update_state(uint8_t type);
int validate_checksum(gbnhdr *frame);
void gbnhdr_to_buffer(gbnhdr *frame, char *buffer, int buffer_size);
void wrong_packet_error(uint8_t expected, uint8_t received);
void send_packet(gbnhdr *frame, int sockfd, uint8_t type, uint8_t seqnum, int data_len);
void buffer_to_gbnhdr(gbnhdr *frame, char *buffer, int buffer_size);
int is_frame_ok(int rcvd_bytes, uint8_t type);
int out_of_window(uint8_t seqnum, uint8_t last_acked_frame, uint8_t window_size);
int rcv(int sockfd,
		gbnhdr *frame,
		struct sockaddr *client,
		socklen_t *socklen,
		uint8_t type);

void reset_frame_counter(uint8_t expected,
						uint8_t received,
						int* attempts,
						int* i,
						uint8_t* frame_counter,
						uint8_t last_acked_frame);

void set_frame_counter( uint8_t received,
						uint8_t* frame_counter,
						uint8_t* last_acked_frame,
						int* i,
						int* attempts);

int is_frame_correct(int rcvd_bytes,
					uint8_t type,
					uint8_t expected_type,
					uint8_t last_acked_frame,
					uint8_t expected_seqnum,
					uint8_t received_seqnum);

#endif
