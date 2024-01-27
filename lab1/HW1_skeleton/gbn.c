#include "gbn.h"

state_t s;

uint16_t checksum(uint16_t *buf, int nwords)
{
	uint32_t sum;

	for (sum = 0; nwords > 0; nwords--)
		sum += *buf++;
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	return ~sum;
}

ssize_t gbn_send(int sockfd, const void *buf, size_t len, int flags){

	/* TODO: Your code here. */

	/* Hint: Check the data length field 'len'.
	 *       If it is > DATALEN, you will have to split the data
	 *       up into multiple packets - you don't have to worry
	 *       about getting more than N * DATALEN.
	 */

	return(-1);
}

ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){



	return(-1);
}

int gbn_close(int sockfd){
	int result = close(sockfd);
	if (result == -1){
		perror("socket could not close.");
		exit(-1);
	}

	update_state(CLOSED);
	printf("Socket %d closed.\n", sockfd);
	return(result);
}

/* Used by sender (client) to establish connection */
int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen){

	int attempts = 0;
	gbnhdr *packet = malloc(sizeof(gbnhdr));

	/*----- Setting the destination (server) details -----*/
	memcpy(&s.dest_addr, server, socklen);
	s.dest_sock_len = socklen;

	printf("Server info set.\n");

	/*----- Sending the SYN packet -----*/
	while (s.curr_state != ESTABLISHED)
	{
		/* Try MAX_ATTEMPTS times */
		if (attempts > MAX_ATTEMPTS)
		{
			printf("TIMEOUT Connection could not be established.\n");
			exit(-1);
		}

		/* If the connection is closed, send a SYN packet */
		else if (s.curr_state == CLOSED)
		{
			attempts++;
			send_packet(packet, sockfd, SYN, 0);
			printf("SYN packet sent.\n");
			update_state(SYN_SENT);
		}

		/* If the connection is in SYN_SENT state, wait for a SYNACK packet */
		else if (s.curr_state == SYN_SENT)
		{
			/* TODO: Set the timeout value to TIMEOUT seconds */

			rcv_and_validate(sockfd, packet, s.dest_addr, &s.dest_sock_len, SYNACK);
			printf("SYNACK packet received.\n");
			update_state(ESTABLISHED);
		}
	}

	free(packet);
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

	gbnhdr *packet = malloc(sizeof(gbnhdr));

	/*----- Waiting for a SYN packet -----*/
	rcv_and_validate(sockfd, packet, client, socklen, SYN);
	printf("SYN packet received.\n");
	update_state(SYN_RCVD);

	/*----- Setting the destination (client) details -----*/
	memcpy(&s.dest_addr, client, *socklen);
	s.dest_sock_len = *socklen;

	/*----- Sending a SYNACK packet -----*/
	send_packet(packet, sockfd, SYNACK, 0);
	update_state(ESTABLISHED);
	printf("SYNACK packet sent.\n");

	free(packet);
	return(0);
}

ssize_t maybe_recvfrom(int s, char *buf, size_t len, int flags,
					   struct sockaddr *from, socklen_t *fromlen){

	/*----- Packet not lost -----*/
	if (rand() > LOSS_PROB*RAND_MAX){

		/*----- Receiving the packet -----*/
		int retval = recvfrom(s, buf, len, flags, from, fromlen);

		/*----- Packet corrupted -----*/
		if (rand() < CORR_PROB*RAND_MAX){
			/*----- Selecting a random byte inside the packet -----*/
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

            /*----- Selecting a random byte inside the packet -----*/
            int index = (int)((len-1)*rand()/(RAND_MAX + 1.0));

            /*----- Inverting a bit -----*/
            char c = buffer[index];
            if (c & 0x01)
                c &= 0xFE;
            else
                c |= 0x01;
            buffer[index] = c;
        }

        /*----- Sending the packet -----*/
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

void gbnhdr_to_uint16(uint16_t *buf, gbnhdr *packet) {
	buf[0] = (uint16_t)(((uint16_t)packet->type << 8) | (uint16_t)packet->seqnum);
	buf[1] = packet->seqnum;
	int i;
    for (i = 0; i < DATALEN/2; i++)
        buf[i + 2] = (uint16_t)(((uint16_t)packet->data[2*i] << 8) | (uint16_t)packet->data[2*i+1]);
}

void set_checksum(gbnhdr *packet) {
	packet->checksum = 0;
	uint16_t *buf = malloc(sizeof(*packet));
	gbnhdr_to_uint16(buf, packet);
	packet->checksum = checksum(buf, sizeof(*packet));
	free(buf);
}

int validate_checksum(gbnhdr *packet) {
	uint16_t *buf = malloc(sizeof(*packet));
	gbnhdr_to_uint16(buf, packet);
	uint16_t resultsum = checksum(buf, sizeof(*packet));
	free(buf);
	if (resultsum == packet->checksum)
		return 1;
	else
		return 0;
}

void send_packet(gbnhdr *packet, int sockfd, uint8_t type, uint8_t seqnum) {
	packet->type = type;
	set_checksum(packet);
	packet->seqnum = seqnum;

	if (maybe_sendto(sockfd, packet, sizeof(*packet), 0, s.dest_addr, s.dest_sock_len) == -1) {
		perror("gbn_send: DATA");
		exit(-1);
	}
}

void buffer_to_gbnhdr(gbnhdr *packet, char *buffer, int buffer_size) {
	int id = 0;
	packet->type = buffer[id++];
	packet->seqnum = buffer[id++];

	/* convert checksum from char array to uint16_t */
	memcpy(&packet->checksum, &buffer[id], sizeof(packet->checksum));
	packet->checksum = ntohs(packet->checksum);
	id += sizeof(packet->checksum);

	memcpy(packet->data, &buffer[id], buffer_size - id);
}

void rcv_and_validate(int sockfd, gbnhdr *packet, struct sockaddr *from,
					  socklen_t *socklen, uint8_t type) {
	char *buffer = malloc(sizeof(*packet));

	if (maybe_recvfrom(sockfd, buffer, sizeof(*packet), 0, from, socklen) == -1) {
		perror("gbn_recv: DATA");
		exit(-1);
	}
	printf("%d packet received.\n", type);

	buffer_to_gbnhdr(packet, buffer, sizeof(*packet));
	printf("Packet converted to gbnhdr.\n");

	if (!validate_checksum(packet)) {
		printf("%d packet corrupted.\n", type);
		exit(-1);
	}
	printf("%d packet validated.\n", type);

	if (packet->type != type) {
		printf("%d packet expected. Recieved %d...\n", type, packet->type);
		exit(-1);
	}
	printf("%d packet type validated.\n", type);
}