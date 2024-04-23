# Report

## What is this?

The Go-Back-N protocol is a reliable data transfer protocol used in networking. It allows the sender to continuously send a specified number of frames, known as the window size, without waiting for individual acknowledgements from the receiver. If the sender does not receive an acknowledgement within a certain time period, it assumes that the frame was lost or damaged and retransmits all the frames in the current window. This protocol ensures that all frames are successfully received by the receiver, but it may result in duplicate frames being received if an acknowledgement is lost or delayed. The Go-Back-N protocol strikes a balance between efficiency and reliability by allowing the sender to transmit multiple frames without waiting for individual acknowledgements, while still ensuring that all frames are successfully received.

## How to use this

``./sender <hostname> <port> <filename>``
``./receiver <port> <filename>``

## Sender Side State Machine

* CLOSED: This is where our Go-Back-N protocol will always be initiated on startup. After starting the program, the sender will issue a frame to the server with the SYN frame. Right after sending it, it will move to the SYN_SENT state.
* SYN_SENT:
  * If the sender is unable to receive a SYN_ACK frame back, it will time out within 1 second and return back to the CLOSED state. The sender will try to resend the SYN frame again MAX_ATTEMPTS times. The sender will then terminate the program if it is unable to connect to the receiver.
  * If the sender is able to receive a SYN_ACK frame type, it will become ESTABLISHED.
* ESTABLISH: The sender side implementation in the Go-Back-N protocol involves sending multiple DATA frame without waiting for individual acknowledgements. The sender starts with window size 1. After sending the frame, the sender will set a timer to wait for DATAACK results to come back. This timer is reset whenever a frame is received. The frames are processed like this:
  * Each frame is sent with a sequence number that is incremented by 1 and modulo 256 because of the 8-bit sequence number.
  * If there is no timeout, it will send multiple DATA frames doubling it in every iteration (uptill 16) unless there has previously been a timeout in which case the window size will remain the same.
  * If it was interrupted by an alarm, the sender will resend the DATA frame after the last acknowledged frame and reduce the window size by half.
  * If all of the expected frame are not received in time, the window is updated to reflect the last ACK'ed frame.
  * If the DATAACK frame received are outside of the window range, the sender resets the window to the last ACK'ed frame and sends the frames again.
  * The sender waits until the last frame is acknowledged before sending the FIN frame.
* FIN_SENT: Once the sender is finished reading the file, it will send a FIN type frame to the server. After this, it will wait for the FIN_ACK frame and return to the initial CLOSED state.

## Receiver Side Finite State Machine

* CLOSED: In this state, the receiver is essentially waiting for any SYN frame from any sender.
* SYN_RCVD: When the receiver receives a SYN frame, it will transition to the SYN_RCVD state. Once the receiver
  transitions to this stage, it sends a SYN_ACK frame back to the receiver and goes to the ESTABLISHED state.
* ESTABLISHED: In the established state, the receiver is essentially processing any DATA frame it receives. The logic is:
  * If the received frame is not equal to the expected sequence number (i.e. one number higher than the last received frame), it will discard the frame. At startup the expected sequence number is 0.
  * The receiver will send a DATAACK frame with the sequence number of the highest previous consecutive frame received.
  * If the received frame is a FIN frame, it will transition to FIN_RCVD.
* FIN_RCVD: In this state, the receiver simply sends a FIN_ACK frame and transition to the CLOSED state.

## How to test this

Put Tests folder in the root directory and run:
`` . ./tests.sh ``

## Known issues

1. There is a situation where the receiver successfully receives the SYN frame, but the sender does not receive the SYN_ACK frame. In this case, the sender will keep sending the SYN frame until it reaches the MAX_ATTEMPTS limit. The receiver will not receive the SYN frame again because it has already received it and established a connection with a certain sender. This will become a deadlock which will not be resolved until the receiver is restarted.

## Some issues in implementation

1. The ``signal()`` function is not supported any more. I was not aware of this and had to switch to ``sigaction()`` function.
2. There was a need to convert the ``gbnhdr struct`` to a char array to send it over the network.