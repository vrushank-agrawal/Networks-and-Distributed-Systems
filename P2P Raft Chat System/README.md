# P2P Networking

Environment: C++20, Ubuntu 20.04
Names: Madhav Mathur (mm3338), Vrushank Agrawal (vca4)

## High Level Details

The code is separated into three main classes: Server, StatusMessage and RumorMessage. The Server class is responsible for handling the networking of the peers and is implemented in the server.cpp file. The StatusMessage and RumorMessage classes are used to represent the two types of messages that can be sent between peers and both are implemented in the messages.cpp file. The headers for the classes are in the headers folder and are called through Makefile for compilation and linking.

## process

The main.cpp file is linked to process which is called by the proxy. This file binds the process to the specified port and listens for incoming connections. The ./process command accepts three arguments but effectively drops the first (pid) and second (nodes) one while only using the third argument which is the port number. This is the minimum information required to run the p2p network because we have a hard assumption that every peer can only have 2 neighbors and 1 proxy connection where the 2 neighbors are its adjacent ports.

## stopall

The stopall.c file is simply a placeholder for the proxy script. When the exit command is entered in the proxy, the servers each connected to the proxy receive a zero message which they recognize and use to exit their respective processes. In other words, the servers recognize that the proxy is dying so they should die as well.

## Neighbors

We have the hard assumption that every peer can only have 2 neighbors which are its adjacent ports. Exploiting this, we ensure that every peer only tries to connect to its left neighbor while listening for active connections from its right neighbor. Doing this, the first peer will always try to connect to its left neighbor but will never connect because it doesn't exist. Whereas, the last peer will always listen for its right neighbor but will never receive a connection because it doesn't exist. This is a simple way to ensure that the network is connected and that every peer has exactly 2 neighbors.

## Threads

The server class uses 4 threads in total. The first thread is used to connect and listen to the left neighbor and detached. The second thread is then used to implement anti-entropy and detached. The main thread meanwhile listens for incoming connections from the right neighbor and the proxy. Whenever a new connection is received, the server recognizes if it is from the right neighbor because the right neighbor sends a "whoami" message. This allows each server to differentiate between the right neighbor and the proxy. The main thread then creates a new thread to handle the connection, detaches it, and then listens for the next connection.

## Gossip Protocol

There are two types of messages in the gossip protocol: StatusMessage and RumorMessage. The StatusMessage is used to inform the neighbor of the status of all the messages the current peer. This information is stored as a dictionary of the form {port: sequence_number} where the port is the port of the peer and the sequence_number is the maximum number of contiguous messages received from that peer. This data is sent in a concatenated string of the format "status port:sequence_number,port:sequence_number,...". The RumorMessage is used to send a message to the neighbor which is of the format "rumor from:___,chatText:___,seqNo:___" where from is the origin of the message, chatText is the message and seqNo is the sequence number of the message.

## Anti-Entropy

The anti-entropy is implemented every 5 seconds. This method is spun as a separate thread in each peer after they start connecting to the left neighbor. Within anti-entropy, the peer randomly chooses from the left and right neighbor and checks if it is connected. If it is connected, it sends a status message to the neighbor. The peer sends the minimum amount of information needed which is the maximum sequence number of the messages it has received from each peer.

## RumorMongering

Once the anti-entropy message is sent. The peer either receives another status message or rumor message based on the chatLog of the receiving peer. If the peer receives a status message, it checks which messages the receiving peer does not have and sends them in a for loop with a wait time of 100 milliseconds to ensure the receiving peer process the first message correctly. The peer sends these messages in ascending order of their sequence number in ascending order of the port number and only ascends the missing messages for the receiving peer. This ensures optimality and a strict order of messages in increasing order from each origin. The peer also only adds messages if they are in order. Out of order messages are dropped. Moreover, sending all rumor messages in a loop ensures a quicker update for the peers.

## Analysis of vulnerabilities

There are certain assumptions that were made during development of this project which may be exploited as potential vulnerabilities. We have implemented defensive code in our program to fix and handle such edge cases.

1. Out of order messageIDs are rejected by the server. We make sure that the incoming messageID to the server is exactly one higher than the maximum ID number we have seen from that origin before.
2. Empty chat logs are handled, so that the proxy program works as expected even after an empty chat log is returned. For example, if “1 get chatLog” is called, but 1’s chatLog is empty, we return “chatLog \x01\n” so that the proxy input while loop continues, and the message output to the proxy client is an empty string which is what the user expects.
3. Race condition when a chatLog is being written to by the proxy while another server is fetching it during anti-entropy. We are handling this using shared_lock (shared mutex) for reads and unique_lock (exclusive mutex) for writes. This ensures that at any given time, only one thread can write to the chatLog and status map data structures, while multiple threads may read from them.
4. Handling graceful as well as abrupt disconnections. While we have implemented the controlled flow kicked off with the “exit” command, we have also implemented our server instances such that they are resilient to abrupt disconnections from the proxy (for e.g., user typing Ctrl-C in the executing terminal). When such signals are received, the servers gracefully terminate their sockets and free heap resources to prevent memory leaks and inactive ports.
5. Reconnecting with crashed servers. When a server’s left neighbor and right neighbor crashes, the server is able to recognize this, and continues to try and reconnect with them. Therefore, if/when the neighbors are brought back online, the server is able to reconnect with them. This enables the functionality of being able to crash and start servers without worrying if the remaining servers would be able to work once/if they are restarted.
6. 2 rapid back-to-back rumor messages can appear as 1 concatenated rumor message to the recipient. To avoid this, our server program waits 100ms before sending each rumor message to its neighbor, which solves this edge case.

## Changes to proxy.py

Line 53 edited to self.buffer += data.decode('utf-8')

- This is because of a data encoding issue. The original code was not able to decode messages appropriately and was throwing an error.

## Screenshot explanation

In the above running instance of proxy, we demonstrate the following functionality:

- Starting multiple servers

- Sending messages with consecutive message IDs to a single server

- Timely anti-entropy that kicks off rumor mongering with random neighbors to share chatlog.

- Graceful handling of out of order message IDs by the server.

- Repeated crashing and restarting of multiple servers, multiple times.

- Crashing middle servers, adding new messages to the end servers, restarting middle servers and ensuring the full message history propagates to the newly restarted servers.

- Crashing end servers, adding new messages to the middle servers, restarting end servers and ensuring the full message history propagates to the newly restarted servers.

- Crashing all servers and losing all message history and practically restarting fresh within the same proxy instance (not captured in this screenshot)

- Graceful handling of empty chat log

- Graceful exiting of the program and terminating all connections.
