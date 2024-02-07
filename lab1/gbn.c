#include "gbn.h"

state_t s;

/* The original checksum function was modified to
 * work with the gbnhdr struct for convenience */
uint16_t checksum(gbnhdr *frame)
{
	/* We add 1 because type and seqnum are 2 bytes together */
	int nwords = 1 + DATALEN/2;
	uint16_t buf[nwords];
	/* The frame is converted to a buffer of 16-bit words */
	buf[0] = (uint16_t)frame->seqnum + ((uint16_t)frame->type << 8);
	int offset = 1, byte = 0;
    for (; byte < DATALEN/2; byte++) {
		buf[offset++] = (uint16_t)frame->data[byte*2 + 1] +
						((uint16_t)frame->data[byte*2] << 8);
	}

	/* Original checksum algorithm */
	uint32_t sum;

	for (sum = 0; nwords > 0; nwords--)
		sum += buf[nwords - 1];
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}

ssize_t gbn_send(int sockfd, const void *buf, size_t len, int flags){

	/* Hint: Check the data length field 'len'.
	 *       If it is > DATALEN, you will have to split the data
	 *       up into multiple packets - you don't have to worry
	 *       about getting more than N * DATALEN.
	 */

	gbnhdr *frame = calloc(1, sizeof(gbnhdr));


	return(0);
}

ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){

	gbnhdr *frame = calloc(1, sizeof(gbnhdr));

	/*----- Waiting for a frame -----*/
	rcv(sockfd, frame, &s.dest_addr, &s.dest_sock_len, DATA);

	/*----- Received a DATA frame -----*/
	if (frame->type == DATA) {
		printf("DATA frame received.\n");
		send_packet(frame, sockfd, DATAACK, frame->seqnum);
	}

	/*----- The connection is being closed -----*/
	else if (frame->type == FIN) {
		printf("FIN frame received.\n");
		update_state(FIN_RCVD);
	}

	/*----- This should not happen -----*/
	else wrong_packet_error(DATA, frame->type);

	free(frame);
	if (s.curr_state == FIN_RCVD)
		return(0);
	return(1);
}

int gbn_close(int sockfd){

	int result, attempts = 0;
	gbnhdr *frame = calloc(1, sizeof(gbnhdr));

	/*----- Sending the FIN frame -----*/
	while (s.curr_state != CLOSED)
	{
		/* Try MAX_ATTEMPTS times */
		if (attempts > MAX_ATTEMPTS)
		{
			printf("TIMEOUT Connection could not be closed.\n");
			exit(-1);
		}

		/* This is the sender's side (client) */
		/* If the connection is established, send a FIN frame */
		else if (s.curr_state == ESTABLISHED)
		{
			attempts++;
			send_packet(frame, sockfd, FIN, 0);
			printf("FIN frame sent.\n");
			update_state(FIN_SENT);
		}

		/* If the connection is in FIN_SENT state, wait for a FINACK frame */
		else if (s.curr_state == FIN_SENT)
		{
			rcv(sockfd, frame, &s.dest_addr, &s.dest_sock_len, FINACK);
			if (frame->type != FINACK) wrong_packet_error(FINACK, frame->type);
			printf("FINACK frame received.\n");
			if ((result = close(sockfd)) == -1){
				perror("socket could not close.");
				exit(-1);
			}
			update_state(CLOSED);
		}

		/* This is the receiver's side (server) */
		/* If the connection is in FIN_RCVD state, send a FINACK frame */
		else if (s.curr_state == FIN_RCVD)
		{
			send_packet(frame, sockfd, FINACK, 0);
			printf("FINACK frame sent.\n");
			if ((result = close(sockfd)) == -1){
				perror("socket could not close.");
				exit(-1);
			}
			update_state(CLOSED);
		}

		/* Apocalyptic scenario */
		else {
			printf("gbn_close: This should never happen.\n");
			exit(-1);
		}

	}

	printf("Socket %d closed.\n", sockfd);
	free(frame);
	return(result);
}

/* Used by sender (client) to establish connection */
int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen){

	int attempts = 0;
	gbnhdr *frame = calloc(1, sizeof(gbnhdr));

	/*----- Setting the destination (server) details -----*/
	memcpy(&s.dest_addr, server, socklen);
	s.dest_sock_len = socklen;

	/*----- Sending the SYN frame -----*/
	while (s.curr_state != ESTABLISHED)
	{
		/* Try MAX_ATTEMPTS times */
		if (attempts > MAX_ATTEMPTS)
		{
			printf("TIMEOUT Connection could not be established.\n");
			exit(-1);
		}

		/* If the connection is closed, send a SYN frame */
		else if (s.curr_state == CLOSED)
		{
			attempts++;
			send_packet(frame, sockfd, SYN, 0);
			printf("SYN frame sent.\n");
			update_state(SYN_SENT);
		}

		/* If the connection is in SYN_SENT state, wait for a SYNACK frame */
		else if (s.curr_state == SYN_SENT)
		{
			/* TODO: Set the timeout value to TIMEOUT seconds */

			rcv(sockfd, frame, &s.dest_addr, &s.dest_sock_len, SYNACK);
			if (frame->type != SYNACK) wrong_packet_error(SYNACK, frame->type);
			printf("SYNACK frame received.\n");
			update_state(ESTABLISHED);
		}
	}

	free(frame);
	return(0);
}

int gbn_listen(int sockfd, int backlog){
	return(0);
}

int gbn_bind(int sockfd, const struct sockaddr *server, socklen_t socklen){

	int result = bind(sockfd, server, socklen);
	if (result == -1){
		perror("socket could not bind.");
		exit(-1);
	}

	printf("Socket %d binded.\n", sockfd);
	return(result);
}

int gbn_socket(int domain, int type, int protocol){

	/*----- Randomizing the seed. This is used by the rand() function -----*/
	srand((unsigned)time(0));

	int sockfd = socket(domain, type, protocol);
	if (sockfd == -1){
		perror("socket not created");
		exit(-1);
	}

	/*----- If socket is opened the connection is still closed -----*/
	update_state(CLOSED);
	printf("Socket %d created.\n", sockfd);
	return(sockfd);
}

int gbn_accept(int sockfd, struct sockaddr *client, socklen_t *socklen){

	gbnhdr *frame = calloc(1, sizeof(gbnhdr));

	/*----- Waiting for a SYN frame -----*/
	rcv(sockfd, frame, client, socklen, SYN);
	if (frame->type != SYN) wrong_packet_error(SYN, frame->type);
	printf("SYN frame received.\n");
	update_state(SYN_RCVD);

	/*----- Setting the destination (client) details -----*/
	memcpy(&s.dest_addr, client, *socklen);
	s.dest_sock_len = *socklen;

	/*----- Sending a SYNACK frame -----*/
	send_packet(frame, sockfd, SYNACK, 0);
	update_state(ESTABLISHED);
	printf("SYNACK frame sent.\n");

	free(frame);
	return(0);
}

ssize_t maybe_recvfrom(int s, char *buf, size_t len, int flags,
					   struct sockaddr *from, socklen_t *fromlen){

	/*----- Packet not lost -----*/
	if (rand() > LOSS_PROB*RAND_MAX){

		/*----- Receiving the frame -----*/
		int retval = recvfrom(s, buf, len, flags, from, fromlen);

		/*----- Packet corrupted -----*/
		if (rand() < CORR_PROB*RAND_MAX){
			/*----- Selecting a random byte inside the frame -----*/
			int index = (int)((len-1)*rand()/(RAND_MAX + 1.0));

			/*----- Inverting a bit -----*/
			char c = buf[index];
			if (c & 0x01)
				c &= 0xFE;
			else
				c |= 0x01;
			buf[index] = c;
		}

		return retval;
	}
	/*----- Packet lost -----*/
	return(len);  /* Simulate a success */
}

ssize_t maybe_sendto(int s, const void *buf, size_t len, int flags, \
                     const struct sockaddr *to, socklen_t tolen){

    char *buffer = malloc(len);
    memcpy(buffer, buf, len);

    /*----- Packet not lost -----*/
    if (rand() > LOSS_PROB*RAND_MAX){
        /*----- Packet corrupted -----*/
        if (rand() < CORR_PROB*RAND_MAX){

            /*----- Selecting a random byte inside the frame -----*/
            int index = (int)((len-1)*rand()/(RAND_MAX + 1.0));

            /*----- Inverting a bit -----*/
            char c = buffer[index];
            if (c & 0x01)
                c &= 0xFE;
            else
                c |= 0x01;
            buffer[index] = c;
        }

        /*----- Sending the frame -----*/
        int retval = sendto(s, buffer, len, flags, to, tolen);
        free(buffer);
        return retval;
    }
    /*----- Packet lost -----*/
    else
        return(len);  /* Simulate a success */
}

/******************************************************************
*                       Auxiliary Functions                       *
*******************************************************************/

void update_state(uint8_t state) {
	s.curr_state = state;
}

int validate_checksum(gbnhdr *frame) {
	uint16_t resultsum = checksum(frame);
	printf("Checksum: %d | Resultsum: %d\n", frame->checksum, resultsum);
	if (resultsum == frame->checksum)
		return 1;
	else
		return 0;
}

void send_packet(gbnhdr *frame, int sockfd, uint8_t type, uint8_t seqnum) {
	frame->type = type;
	frame->checksum = checksum(frame);
	frame->seqnum = seqnum;

	if (maybe_sendto(sockfd, frame, sizeof(*frame), 0, &s.dest_addr, s.dest_sock_len) == -1) {
		perror("gbn_send: DATA");
		exit(-1);
	}
}

void buffer_to_gbnhdr(gbnhdr *frame, char *buffer, int buffer_size) {
	int id = 0, i = 0;
	frame->type = buffer[id++];
	frame->seqnum = buffer[id++];

	/* The checksum is a 16-bit value, so we need to convert it
	 * from 2 bytes to a uint16_t */
	char *ptr = (char *)&frame->checksum;
	for (i = 0; i < sizeof(frame->checksum); i++)
		ptr[i] = buffer[id++];

	/* The data is a DATALEN array */
	for (i = 0; i < DATALEN; i++)
		frame->data[i] = buffer[id++];
}

/* Used for debugging */
const char *states[] = {
	"SYN",
	"SYNACK",
	"DATA",
	"DATAACK",
	"FIN",
	"FINACK",
	"RST"
};

void rcv(int sockfd, gbnhdr *frame, struct sockaddr *from, socklen_t *socklen, uint8_t type) {
	char *buffer = malloc(sizeof(*frame));

	if (maybe_recvfrom(sockfd, buffer, sizeof(*frame), 0, from, socklen) == -1) {
		perror("gbn_recv: DATA");
		exit(-1);
	}
	buffer_to_gbnhdr(frame, buffer, sizeof(*frame));

	if (!validate_checksum(frame)) {
		printf("%s frame corrupted.\n", states[type]);
		exit(-1);
	}

	free(buffer);
}

void wrong_packet_error(uint8_t expected, uint8_t received) {
	printf("Expected %s frame, received %s frame.\n", states[expected], states[received]);
	exit(-1);
}