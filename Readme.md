# Networks and Distributed Systems

This repository contains three projects, one related to networking while the other two are distributed systems projects. You can find more information about each project in their respective directories by clicking on the hyperlinks.

- [Networks and Distributed Systems](#networks-and-distributed-systems)
  - [Go-Back-N File Sharing](#go-back-n-file-sharing)
  - [MapReduce](#mapreduce)
  - [P2P Distributed Chat w Raft Consensus](#p2p-distributed-chat-w-raft-consensus)

## [Go-Back-N File Sharing](Go-Back-N%20File%20Sharing/README.md)

This project implements the Go-back-n (GBN) protocol within a real-world UNIX system environment. It addresses issues typical of unreliable network links, such as packet reordering, loss, and corruption. This program ensures that data transmitted from one end of the connection appears exactly as sent at the other end. While the implementation does not aim to achieve full-duplex data transmission, it provides the illusion of a byte stream between the communicating processes. Additionally, a simple congestion control mechanism is implemented, where the transmission rate adjusts dynamically based on packet loss. Specifically, the protocol slows down transmission speed (e.g., Go-back-1) when packets are dropped and speeds up (e.g., Go-back-2, Go-back-4) upon successful receipt of acknowledgments. Notably, only one end of the connection sends data, while the other end solely sends acknowledgments back to the sender.

## [MapReduce](MapReduce/Readme.md)

The MapReduce software implemented using multithreading involves creating a system with a master node coordinating tasks on multiple worker nodes. The software follows the structure outlined in the original MapReduce paper. The program call initializes a master node and a specified number of worker processes. The MapReduce process operates on files from the input directory, utilizing a specified number of reduce tasks and assigning a map task to each file. The results, including temporary files, are written to the output directory.

## [P2P Distributed Chat w Raft Consensus](P2P%20Distributed%20Chat%20w%20Raft%20Consensus/README.md)

The project is a distributed chat room that utilizes the Raft consensus protocol to ensure message ordering consistency among users. Without a consensus protocol, a peer-to-peer chat system lacks message order guarantee. However, this project addresses scenarios like heated online debates where message ordering is crucial. The goal is to ensure that all users see messages in the same order despite potential peer or communication failures. The implementation is a variation of the Raft protocol tailored to support this functionality.

The [Raft consensus protocol](https://raft.github.io/raft.pdf), a recently proposed alternative to Paxos, offers similar fault-tolerance guarantees but features a simpler communication algorithm.
