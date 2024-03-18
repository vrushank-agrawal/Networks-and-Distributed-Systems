#include "headers.hpp"

/***********************
 * RumorMessage class  *
 ***********************/

/**
 * @brief Construct a new Rumor Message:: Rumor Message object
 * @param seq Sequence number of the message
 * @param message Message received
*/
RumorMessage::RumorMessage(int seq, std::string message) {
    this->seqNo = seq;
    this->message = message;
    // this->decomposeMessage();
}

/**
 * @brief Decompose the message into chatText and from
*/
void RumorMessage::decomposeMessage() {

}

/**
 * @brief Get the chatText from the message
*/
std::string RumorMessage::getChatText() {
    return this->chatText;
}

/**
 * @brief Get the sender of the message
*/
std::string RumorMessage::getFrom() {
    return this->from;
}

/**
 * @brief Get the sequence number of the message
*/
int RumorMessage::getSeqNo() {
    return this->seqNo;
}

/**
 * @brief Get the message
*/
std::string RumorMessage::getMessage() {
    return this->message;
}


/***********************
 * StatusMessage class *
 ***********************/

StatusMessage::StatusMessage() {}

/**
 * @brief Construct a new Status Message:: Status Message object
 * @param port Port number of the server
*/
StatusMessage::StatusMessage(int port) {
    this->status[port+1] = 0;
    this->status[port-1] = 0;
    this->maxSeqNo = 0;
}

/**
 * @brief Update the status of max messages at the other server
 * @param port Port number of the other server
 * @param seqNo Sequence number of the message
*/
void StatusMessage::updateStatus(int port, int seqNo) {
    this->status[port] = seqNo;
}

/**
 * @brief Add a message to the global chat log of current server
 * @param message Message to be added
 * @param seq Sequence number of the message (also the max sequence number)
*/
void StatusMessage::addMessageToLog(RumorMessage message, int seq) {
    std::cout << "Adding message " << seq << " to chat log" << std::endl;
    this->chatLog.push_back(message);
    this->updateMaxSeqNo(seq);
}

/**
 * @brief Update the maximum sequence number of the chat log
 * @param seqNo Sequence number to be updated
*/
void StatusMessage::updateMaxSeqNo(int seqNo) {
    this->maxSeqNo = seqNo;
}

/**
 * @brief Get the status of the other server
*/
std::map<int, int> StatusMessage::getStatus() {
    return this->status;
}

/**
 * @brief Get the chat log of the server
*/
std::vector<RumorMessage> StatusMessage::getChatLog() {
    return this->chatLog;
}

/**
 * @brief Get the maximum sequence number of the chat log
*/
int StatusMessage::getMaxSeqNo() {
    return this->maxSeqNo;
}

/**
 * @brief Get a specific message from the chat log
 * @param seqNo Sequence number of the message to retrieve
*/
std::string StatusMessage::getMessage(int seqNo) {
    for (RumorMessage message : this->chatLog) {
        if (message.getSeqNo() == seqNo) {
            return message.getMessage();
        }
    }

    /* Message not found */
    std::cerr << "Error: Message "<< seqNo << " not found in chatLog with maxSeqNo " << this->maxSeqNo << std::endl;
    return "";
}