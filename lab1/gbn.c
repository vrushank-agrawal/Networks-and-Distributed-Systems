#include "gbn.h"

state_t s;

/* The original checksum function was modified to
 * work with the gbnhdr struct for convenience */
uint16_t checksum(gbnhdr *packet)
{
	/* We add 1 because type and seqnum are 2 bytes together */
	int nwords = 1 + DATALEN/2;
	uint16_t buf[nwords];
	/* The packet is converted to a buffer of 16-bit words */
	buf[0] = (uint16_t)packet->seqnum + ((uint16_t)packet->type << 8);
	int offset = 1, byte = 0;
    for (; byte < DATALEN/2; byte++) {
		buf[offset++] = (uint16_t)packet->data[byte*2 + 1] +
						((uint16_t)packet->data[byte*2] << 8);
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

	gbnhdr *packet = calloc(1, sizeof(gbnhdr));


	return(0);
}

ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){

	gbnhdr *packet = calloc(1, sizeof(gbnhdr));

	/*----- Waiting for a packet -----*/
	rcv(sockfd, packet, &s.dest_addr, &s.dest_sock_len, DATA);

	/*----- Received a DATA packet -----*/
	if (packet->type == DATA) {
		printf("DATA packet received.\n");
		send_packet(packet, sockfd, DATAACK, packet->seqnum);
	}

	/*----- The connection is being closed -----*/
	else if (packet->type == FIN) {
		printf("FIN packet received.\n");
		update_state(FIN_RCVD);
	}

	/*----- This should not happen -----*/
	else wrong_packet_error(DATA, packet->type);

	free(packet);
	if (s.curr_state == FIN_RCVD)
		return(0);
	return(1);
}

int gbn_close(int sockfd){

	int result, attempts = 0;
	gbnhdr *packet = calloc(1, sizeof(gbnhdr));

	/*----- Sending the FIN packet -----*/
	while (s.curr_state != CLOSED)
	{
		/* Try MAX_ATTEMPTS times */
		if (attempts > MAX_ATTEMPTS)
		{
			printf("TIMEOUT Connection could not be closed.\n");
			exit(-1);
		}

		/* This is the sender's side (client) */
		/* If the connection is established, send a FIN packet */
		else if (s.curr_state == ESTABLISHED)
		{
			attempts++;
			send_packet(packet, sockfd, FIN, 0);
			printf("FIN packet sent.\n");
			update_state(FIN_SENT);
		}

		/* If the connection is in FIN_SENT state, wait for a FINACK packet */
		else if (s.curr_state == FIN_SENT)
		{
			rcv(sockfd, packet, &s.dest_addr, &s.dest_sock_len, FINACK);
			if (packet->type != FINACK) wrong_packet_error(FINACK, packet->type);
			printf("FINACK packet received.\n");
			if ((result = close(sockfd)) == -1){
				perror("socket could not close.");
				exit(-1);
			}
			update_state(CLOSED);
		}

		/* This is the receiver's side (server) */
		/* If the connection is in FIN_RCVD state, send a FINACK packet */
		else if (s.curr_state == FIN_RCVD)
		{
			send_packet(packet, sockfd, FINACK, 0);
			printf("FINACK packet sent.\n");
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
	free(packet);
	return(result);
}

/* Used by sender (client) to establish connection */
int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen){

	int attempts = 0;
	gbnhdr *packet = calloc(1, sizeof(gbnhdr));

	/*----- Setting the destination (server) details -----*/
	memcpy(&s.dest_addr, server, socklen);
	s.dest_sock_len = socklen;

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

			rcv(sockfd, packet, &s.dest_addr, &s.dest_sock_len, SYNACK);
			if (packet->type != SYNACK) wrong_packet_error(SYNACK, packet->type);
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

	gbnhdr *packet = calloc(1, sizeof(gbnhdr));

	/*----- Waiting for a SYN packet -----*/
	rcv(sockfd, packet, client, socklen, SYN);
	if (packet->type != SYN) wrong_packet_error(SYN, packet->type);
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

int validate_checksum(gbnhdr *packet) {
	uint16_t resultsum = checksum(packet);
	printf("Checksum: %d | Resultsum: %d\n", packet->checksum, resultsum);
	if (resultsum == packet->checksum)
		return 1;
	else
		return 0;
}

void send_packet(gbnhdr *packet, int sockfd, uint8_t type, uint8_t seqnum) {
	packet->type = type;
	packet->checksum = checksum(packet);
	packet->seqnum = seqnum;

	if (maybe_sendto(sockfd, packet, sizeof(*packet), 0, &s.dest_addr, s.dest_sock_len) == -1) {
		perror("gbn_send: DATA");
		exit(-1);
	}
}

void buffer_to_gbnhdr(gbnhdr *packet, char *buffer, int buffer_size) {
	int id = 0, i = 0;
	packet->type = buffer[id++];
	packet->seqnum = buffer[id++];

	/* The checksum is a 16-bit value, so we need to convert it
	 * from 2 bytes to a uint16_t */
	char *ptr = (char *)&packet->checksum;
	for (i = 0; i < sizeof(packet->checksum); i++)
		ptr[i] = buffer[id++];

	/* The data is a DATALEN array */
	for (i = 0; i < DATALEN; i++)
		packet->data[i] = buffer[id++];
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

void rcv(int sockfd, gbnhdr *packet, struct sockaddr *from, socklen_t *socklen, uint8_t type) {
	char *buffer = malloc(sizeof(*packet));

	if (maybe_recvfrom(sockfd, buffer, sizeof(*packet), 0, from, socklen) == -1) {
		perror("gbn_recv: DATA");
		exit(-1);
	}
	buffer_to_gbnhdr(packet, buffer, sizeof(*packet));

	if (!validate_checksum(packet)) {
		printf("%s packet corrupted.\n", states[type]);
		exit(-1);
	}

	free(buffer);
}

void wrong_packet_error(uint8_t expected, uint8_t received) {
	printf("Expected %s packet, received %s packet.\n", states[expected], states[received]);
	exit(-1);
}