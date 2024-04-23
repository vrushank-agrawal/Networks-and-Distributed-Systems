#include "gbn.h"

state_t s;

/* The timeout handler */
void timeout_handler(int sig) {
	/* reduce the window size by half */
	if (s.window_size > 1) {
		s.window_size /= 2;
		s.timeout = 1;
		printf("Window size reduced to %i\n", s.window_size);
	}
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

	gbnhdr *ack_frame = calloc(1, sizeof(gbnhdr));

	/*----- Splitting the data into multiple frames -----*/
	int num_frames = (len % DATALEN == 0) ? len/DATALEN : len/DATALEN + 1;
	int last_frame_size = (int)len % DATALEN;
	frame_t *frames = calloc(num_frames, sizeof(frame_t));

	/*----- Creating the frames -----*/
	int j;
	for (j = 0; j < num_frames; j++)
	{
		frames[j].frame = calloc(1, sizeof(gbnhdr));
		frames[j].frame_addr = (char *)frames[j].frame;
		frames[j].seqnum = (j) % MAX_SEQNUM;
		frames[j].len = DATALEN;
		memset(frames[j].frame->data, 0, DATALEN);
 		memcpy(frames[j].frame->data, buf + j*DATALEN, DATALEN);
	}
	/*----- The last frame may have a different size -----*/
	if (last_frame_size != 0)
		frames[num_frames-1].len = last_frame_size;

	printf("Number of frames created: %d\n", num_frames);

	uint8_t frame_counter = -1;
	uint8_t last_acked_frame = -1;
	uint8_t expected_ack = 0;
	int attempts = 0;
	int i = 0;
	int rcvd_bytes;

	/*----- Sending the frames until last DATAACK rcvd -----*/
	while (i < num_frames || last_acked_frame != (num_frames - 1) % MAX_SEQNUM)
	{
		if (s.curr_state != ESTABLISHED) {
			printf("gbn_send: Connection not ESTABLISHED.\n");
			exit(-1);
		}

		if (attempts > MAX_ATTEMPTS) {
			printf("Attempts exhausted. data could not be sent.\n");
			exit(-1);
		}

		/******************* Send one window *******************/
		/*----- Send while all acked frames are received -----*/
		if (i < num_frames) {
			do {
				send_packet((gbnhdr *)frames[i].frame_addr, sockfd, DATA, frames[i].seqnum, frames[i].len);
				alarm(TIMEOUT);
				expected_ack = (++frame_counter) % MAX_SEQNUM;
				frame_counter %= MAX_SEQNUM;
				i++;
				printf("After sending frame: %i\ti: %i\tlast_acked_frame: %i\n", expected_ack, i, last_acked_frame);
			} while ((frame_counter < (last_acked_frame + s.window_size) % MAX_SEQNUM) && (i < num_frames));
		}

		/******************* Waiting for an ACK frame *******************/
		rcvd_bytes = rcv(sockfd, ack_frame, &s.dest_addr, &s.dest_sock_len, DATAACK);

		/******************* Check if ACK frame correct *******************/
		/*----- If the frame was not received, reset to zero -----*/
		if (rcvd_bytes == -1 && i == 1) {
			i = 0;
			frame_counter = -1;
		} else if (!is_frame_correct(rcvd_bytes, ack_frame->type, DATAACK, last_acked_frame, expected_ack, ack_frame->seqnum)) {
			reset_frame_counter(expected_ack, ack_frame->seqnum, &attempts, &i, &frame_counter, last_acked_frame);
		} else if (out_of_window(ack_frame->seqnum, last_acked_frame, expected_ack)) {
			set_frame_counter(ack_frame->seqnum, &frame_counter, &last_acked_frame, &i, &attempts);
		} else {
			/*----- The frame is in the window stop the alarm -----*/
			alarm(0);

			/*----- If the timeout was not previously triggered, double the window size -----*/
			if (!s.timeout && s.window_size < 16) {
				s.window_size *= 2;
				printf("Window size doubled to %i\n", s.window_size);
			}

			attempts = 0;
			last_acked_frame = ack_frame->seqnum;
		}
	}
	printf("All frames sent.\n");
	printf("Last acked frame: %i\n", last_acked_frame);

	/*----- Freeing the memory -----*/
	free(ack_frame);
	for (j = 0; j < num_frames; j++)
		free(frames[j].frame);
	free(frames);

	return(0);
}

ssize_t gbn_recv(int sockfd, void *buf, size_t len, int flags){

	gbnhdr *frame = calloc(1, sizeof(gbnhdr));
	int rcvd_bytes = 0;

	while(!flags) {
		/*----- Waiting for a frame -----*/
		rcvd_bytes = rcv(sockfd, frame, &s.dest_addr, &s.dest_sock_len, DATA);
		if (!is_frame_ok(rcvd_bytes, frame->type)) continue;

		/*----- Received a DATA frame -----*/
		if (frame->type == DATA)
		{
			/*----- Check if the frame is correct -----*/
			if (frame->seqnum != (uint8_t)(s.seqnum + 1)) {
				/*----- Sending a DATAACK for the last consecutive frame received -----*/
				send_packet(frame, sockfd, DATAACK, s.seqnum, 0);
				printf("DATAACK frame %i sent.\n", s.seqnum);

				/*----- Continue to the next iteration -----*/
				continue;
			}

			/*----- First 4 bytes are the header -----*/
			rcvd_bytes -= 4;
			memcpy(buf, frame->data, rcvd_bytes);

			/*----- Sending a DATAACK for the last consecutive frame received -----*/
			if ((uint8_t)(frame->seqnum - 1) == s.seqnum)
				/* Store the frame seqnum in the global state */
				s.seqnum = (uint8_t)(frame->seqnum);

			/*----- Sending a DATAACK for the last consecutive frame received -----*/
			send_packet(frame, sockfd, DATAACK, s.seqnum, 0);
			printf("DATAACK frame %i sent.\n", s.seqnum);

			/*----- Exit the loop -----*/
			flags = 1;
		}

		/*----- The connection is being closed -----*/
		else if (frame->type == FIN)
		{
			printf("FIN frame received.\n");
			update_state(FIN_RCVD);

			/*----- Exit the loop -----*/
			flags = 1;
		}

		/*----- This should not happen -----*/
		else wrong_packet_error(DATA, frame->type);
	}

	free(frame);

	/*----- If the connection is being closed, tell the receiver -----*/
	if (s.curr_state == FIN_RCVD)
		return(0);

	return(rcvd_bytes);
}

int gbn_close(int sockfd){

	int result = 0, attempts = 0, rcvd_bytes = 0;
	gbnhdr *frame = calloc(1, sizeof(gbnhdr));

	/*----- Sending the FIN frame -----*/
	while (s.curr_state != CLOSED)
	{
		/* Try MAX_ATTEMPTS times */
		if (attempts > MAX_ATTEMPTS)
		{
			printf("attempts: %i\n", attempts);
			printf("Attempts exhausted. Connection could not be closed.\n");
			exit(-1);
		}

		/******************* Sender's side ********************/
		/* If the connection is established, send a FIN frame */
		else if (s.curr_state == ESTABLISHED)
		{
			attempts++;
			send_packet(frame, sockfd, FIN, 0, 0);
			alarm(TIMEOUT);
			printf("FIN frame sent.\n");
			update_state(FIN_SENT);
		}

		/* If the connection is in FIN_SENT state, wait for a FINACK frame */
		else if (s.curr_state == FIN_SENT)
		{
			rcvd_bytes = rcv(sockfd, frame, &s.dest_addr, &s.dest_sock_len, FINACK);
			if (!is_frame_ok(rcvd_bytes, frame->type)) {
				update_state(ESTABLISHED);
			} else if (frame->type == DATAACK) {
				attempts = 0;
				continue;
			} else if (frame->type != FINACK) {
				update_state(ESTABLISHED);
				wrong_packet_error(FINACK, frame->type);
				attempts--;
			} else {
				printf("FINACK frame received.\n");
				if ((result = close(sockfd)) == -1){
					perror("socket could not close.");
					exit(-1);
				}
				update_state(CLOSED);
				alarm(0);
			}
		}

		/******************* Receiver's side ********************/
		/* If the connection is in FIN_RCVD state, send a FINACK frame */
		else if (s.curr_state == FIN_RCVD)
		{
			send_packet(frame, sockfd, FINACK, 0, 0);
			printf("FINACK frame sent.\n");
			if ((result = close(sockfd)) == -1){
				perror("socket could not close.");
				exit(-1);
			}
			update_state(CLOSED);
		}

		/******************* Apolcalyptic scenario ********************/
		else {
			printf("gbn_close: This should never happen.\n");
			exit(-1);
		}
	}

	printf("Socket %d closed.\n", sockfd);
	free(frame);
	return(result);
}

/* Used by sender to establish connection */
int gbn_connect(int sockfd, const struct sockaddr *server, socklen_t socklen){

	int attempts = 0;
	int rcvd_bytes = 0;
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
			printf("Attempts exhausted. Connection could not be established.\n");
			exit(-1);
		}

		/* If the connection is closed, send a SYN frame */
		else if (s.curr_state == CLOSED)
		{
			attempts++;
			send_packet(frame, sockfd, SYN, 0, 0);
			alarm(TIMEOUT);
			printf("SYN frame sent.\n");
			update_state(SYN_SENT);
		}

		/* If the connection is in SYN_SENT state, wait for a SYNACK frame */
		else if (s.curr_state == SYN_SENT)
		{
			rcvd_bytes = rcv(sockfd, frame, &s.dest_addr, &s.dest_sock_len, SYNACK);

			if (!is_frame_ok(rcvd_bytes, frame->type)) {
				update_state(CLOSED);
			} else if (frame->type != SYNACK) {
				update_state(CLOSED);
				wrong_packet_error(SYNACK, frame->type);
			} else {
				printf("SYNACK frame received.\n");
				update_state(ESTABLISHED);
				alarm(0);
			}
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

	/*----- Setting the state variables -----*/
	s.timeout = 0;
	s.window_size = 1;

	int sockfd = socket(domain, type, protocol);
	if (sockfd == -1){
		perror("socket not created");
		exit(-1);
	}

	/*----- If socket is opened the connection is still closed -----*/
	update_state(CLOSED);
	printf("Socket %d created.\n", sockfd);

	/*----- Setting the timeout handler -----*/
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = timeout_handler;
	sigaction(SIGALRM, &sa, NULL);

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
	send_packet(frame, sockfd, SYNACK, 0, 0);
	update_state(ESTABLISHED);
	s.seqnum = -1;
	printf("SYNACK frame sent.\n");

	free(frame);
	return(sockfd);
}

ssize_t maybe_recvfrom(int s, char *buf, size_t len, int flags,
					   struct sockaddr *from, socklen_t *fromlen){

	/*----- Packet not lost -----*/
	if (rand() > LOSS_PROB*RAND_MAX){

		/*----- Receiving the frame -----*/
		int retval = recvfrom(s, buf, len, flags, from, fromlen);

		/*----- Timeout -----*/
		if (retval == -1) return(-1);

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
	return(-3);  /* Simulate a success */
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
	if (resultsum == frame->checksum)
		return 1;
	else
		return 0;
}

void gbnhdr_to_buffer(gbnhdr *frame, char *buffer, int buffer_size) {
	int id = 0, i = 0;
	buffer[id++] = frame->type;
	buffer[id++] = frame->seqnum;

	/* The checksum is a 16-bit value, so we need to convert it
	 * from a uint16_t to 2 bytes */
	char *ptr = (char *)&frame->checksum;
	for (i = 0; i < sizeof(frame->checksum); i++)
		buffer[id++] = ptr[i];

	/* The data is a DATALEN array */
	for (i = 0; i < buffer_size; i++)
		buffer[id++] = frame->data[i];
}

void send_packet(gbnhdr *frame, int sockfd, uint8_t type, uint8_t seqnum, int data_len) {
	/* If the len is 0, fill the data with 0s */
	if (!data_len) {
		data_len = DATALEN;
		memset(frame->data, 0, DATALEN);
	}
	frame->type = type;
	frame->seqnum = seqnum;
	frame->checksum = checksum(frame);

	/* Convert the frame to a buffer */
	char *buf = calloc(1, data_len + 4);
	gbnhdr_to_buffer(frame, buf, data_len);

	/* Send the buffer */
	if (maybe_sendto(sockfd, buf, data_len + 4, 0, &s.dest_addr, s.dest_sock_len) == -1) {
		perror("gbn_send: DATA");
		exit(-1);
	}

	free(buf);
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

int rcv(int sockfd, gbnhdr *frame, struct sockaddr *from, socklen_t *socklen, uint8_t type) {
	/* Receive the frame */
	char *buffer = malloc(sizeof(*frame));
	memset(buffer, 0, sizeof(*frame));
	int rcvd_bytes = maybe_recvfrom(sockfd, buffer, sizeof(*frame), 0, from, socklen);

	/* Timeout */
	if (rcvd_bytes == -1) return(-1);

	/* If the frame was not received, return -1 */
	if (rcvd_bytes == -3) return(-3);

	/* Convert the buffer to a frame */
	buffer_to_gbnhdr(frame, buffer, sizeof(*frame));

	/* Check if the frame is corrupted and return -2 */
	if (!validate_checksum(frame)) return(-2);

	free(buffer);
	return(rcvd_bytes);
}

/* Display an error message */
void wrong_packet_error(uint8_t expected, uint8_t received) {
	printf("Expected %s frame, received %s frame.\n", states[expected], states[received]);
}

/* If there is an error in receiving DATAACK frames, reset the
 * frame counter and the total frames sent to resend the frames */
void reset_frame_counter(uint8_t expected,
						uint8_t received,
						int* attempts,
						int* i,
						uint8_t* frame_counter,
						uint8_t last_acked_frame)
{
	if (s.window_size > 1) {
		s.window_size = 1;
		s.timeout = 1;
	}

	if (expected == received) {
		*attempts = 0;
		return;
	}

	(*attempts)++;
	/* reset the frame counter */
	*frame_counter = last_acked_frame;

	/* normalize the expected frame window */
	if (expected < last_acked_frame) expected += s.window_size;

	/* reset the total frames sent */
	*i -= (expected - last_acked_frame);
}

/*------------ Check if the frame is in the window ------------*/
int out_of_window(uint8_t received_seqnum, uint8_t last_acked_frame, uint8_t expected_seqnum) {

	if (s.window_size > 1) {
		s.window_size = 1;
		s.timeout = 1;
	}

	/* Frames after the last_acked_frame are not received */
	if (received_seqnum == last_acked_frame) return (1);

	if (received_seqnum >= s.window_size) {
		if ((last_acked_frame < received_seqnum) &&
			(received_seqnum <= last_acked_frame + s.window_size))
			return (0);
	} else {
		if (((expected_seqnum - s.window_size) < received_seqnum + s.window_size) &&
			(received_seqnum <= (last_acked_frame + s.window_size)))
			return (0);
	}

	/* The frame is out of order */
	printf("Expected:%i\t Received: %i\t Window: %i\n", expected_seqnum, received_seqnum, s.window_size);
	return (1);
}

void set_frame_counter( uint8_t received,
						uint8_t* frame_counter,
						uint8_t* last_acked_frame,
						int* i,
						int* attempts)
{
	if (*i > MAX_SEQNUM)
		*i -= (*frame_counter - received);
	else
		*i = received + 1;
	*frame_counter = received;
	*last_acked_frame = received;
	*attempts = 0;

	if (s.window_size > 1) {
		s.window_size = 1;
		s.timeout = 1;
	}
}

int is_frame_correct(int rcvd_bytes, uint8_t type,
					uint8_t expected_type,
					uint8_t last_acked_frame,
					uint8_t expected_seqnum,
					uint8_t received_seqnum)
{
	/* Check if the frame was received */
	if (!is_frame_ok(rcvd_bytes, type))
		return (0);

	/* Check if the frame is of the expected type */
	if (type != expected_type) {
		wrong_packet_error(expected_type, type);
		return(0);
	}

	return(1);
}

int is_frame_ok(int rcvd_bytes, uint8_t type) {
	/* Check if the frame was received */
	if (rcvd_bytes == -3) {
		printf("gbn_rcv: %s frame lost.\n", states[type]);
		return(0);
	}

	/* Check if the frame is corrupted */
	else if (rcvd_bytes == -2) {
		printf("gbn_rcv: %s frame corrupted.\n", states[type]);
		return(0);
	}

	else if (rcvd_bytes == -1) {
		printf("gbn_rcv: Timeout.\n");
		return(0);
	}

	/* The frame is correct */
	return(1);
}