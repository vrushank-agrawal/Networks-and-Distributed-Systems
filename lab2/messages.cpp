#include "headers.hpp"

/***********************
 * RumorMessage class  *
 ***********************/

RumorMessage::RumorMessage() {
    this->seqNo = -1;
    this->message = "";
    this->from = -1;
}

/**
 * @brief Construct a new Rumor Message:: Rumor Message object
 * @param seq Sequence number of the message
 * @param message Message received
*/
RumorMessage::RumorMessage(std::string message) {
    this->message = message;
    this->decomposeMessage();
}

/**
 * @brief Decompose the message into from, chatText and seqNo
 *      Message format: "from:___,chatText:___,seqNo:___"
*/
void RumorMessage::decomposeMessage() {
    std::string delimiter = ",";
    std::string fromDelimiter = "from:";
    std::string chatTextDelimiter = "chatText:";
    std::string seqNoDelimiter = "seqNo:";

    // Find positions of delimiters
    size_t fromPos = this->message.find(fromDelimiter);
    size_t chatTextPos = this->message.find(chatTextDelimiter);
    size_t seqNoPos = this->message.find(seqNoDelimiter);

    // Calculate lengths of substrings
    std::string from = this->message.substr(fromPos + fromDelimiter.length(), chatTextPos - (fromPos + fromDelimiter.length()));
    std::string chatText = this->message.substr(chatTextPos + chatTextDelimiter.length(), seqNoPos - (chatTextPos + chatTextDelimiter.length()) - delimiter.length());
    std::string seqNo = this->message.substr(seqNoPos + seqNoDelimiter.length(), this->message.size() - (seqNoPos + seqNoDelimiter.length()));

    this->from = std::stoi(from);
    this->chatText = chatText;
    this->seqNo = std::stoi(seqNo);
}

/**
 * @brief Get the chatText from the message
*/
std::string RumorMessage::getChatText() {
    return this->chatText;
}

/**
 * @brief Get the message
*/
std::string RumorMessage::getMessage() {
    return this->message;
}

/**
 * @brief Get the sender of the message
*/
int RumorMessage::getFrom() {
    return this->from;
}

/**
 * @brief Get the sequence number of the message
*/
int RumorMessage::getSeqNo() {
    return this->seqNo;
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
    this->statusMap[port] = 0;
    this->chatLog = std::vector<RumorMessage>();
}

/**
 * @brief Update the status of max messages at the other server
 * @param port Port number of the other server
 * @param seqNo Sequence number of the message
*/
void StatusMessage::updateStatus(int port, int seqNo) {
    this->statusMap[port] = seqNo;
}

/**
 * @brief Add a message to the global chat log of current server
 * @param message Message to be added
 * @param seq Sequence number of the message (also the max sequence number)
*/
void StatusMessage::addMessageToLog(RumorMessage message, int seq) {
    std::cout << "Adding message " << seq << " to chat log" << std::endl;
    // std::cout << "Current status map:" << std::endl;
    // for (const auto& pair : this->statusMap) {
    //     std::cout << "Key: " << pair.first << ", Value: " << pair.second << std::endl;
    // }
    if (this->statusMap.find(message.getFrom()) == this->statusMap.end()) {
        if (seq == 1) {
            this->statusMap[message.getFrom()] = 1;     
        } else {
            std::cout << "THIS IS BAD. Trying to write to new origin:" << message.getFrom() << " seq:" << seq << std::endl;
            return;
        }
    } else if (seq == this->statusMap.find(message.getFrom())->second + 1) {
            this->statusMap[message.getFrom()] = seq;
    } else {
        std::cout << "THIS IS BAD. Message received out of order!!! " << "current maxseq: " << this->statusMap.find(message.getFrom())->second << " and recvd seq:" << seq << std::endl;
        return;
    }
    this->chatLog.push_back(message);
}

/**
 * @brief Get the status of the other server
*/
std::map<int, int> StatusMessage::getStatus() {
    return this->statusMap;
}

/**
 * @brief Get the chat log of the server until maxSeqNo elements
*/
std::vector<RumorMessage> StatusMessage::getChatLog() {
    return this->chatLog;
}

/**
 * @brief Get a specific message from the chat log
 * @param seqNo Sequence number of the message to retrieve
*/
std::string StatusMessage::getMessage(int from, int seqNo) {
    for (RumorMessage message : this->chatLog) {
        if ((message.getFrom() == from) && (message.getSeqNo() == seqNo)) {
            return message.getMessage();
        }
    }
    return "";
}