#include "headers.hpp"

/***********************
 * RumorMessage class  *
 ***********************/

RumorMessage::RumorMessage() {
    this->seqNo = -1;
    this->message = "";
    this->from = -1;
}

RumorMessage::RumorMessage(std::string message) {
    this->message = message;
    this->decomposeMessage();
}

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

std::string RumorMessage::getChatText() {
    return this->chatText;
}

std::string RumorMessage::getMessage() {
    return this->message;
}

int RumorMessage::getFrom() {
    return this->from;
}

int RumorMessage::getSeqNo() {
    return this->seqNo;
}

/***********************
 * StatusMessage class *
 ***********************/

StatusMessage::StatusMessage() {}

StatusMessage::StatusMessage(int port) {
    this->statusMap[port] = 0;
    this->chatLog = std::vector<RumorMessage>();
}

void StatusMessage::updateStatus(int port, int seqNo) {
    this->statusMap[port] = seqNo;
}

void StatusMessage::addMessageToLog(RumorMessage message, int seq) {
    std::cout << "Adding message " << seq << " to chat log" << std::endl;
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

std::map<int, int> StatusMessage::getStatus() {
    return this->statusMap;
}

std::vector<RumorMessage> StatusMessage::getChatLog() {
    return this->chatLog;
}

std::string StatusMessage::getMessage(int from, int seqNo) {
    for (RumorMessage message : this->chatLog) {
        if ((message.getFrom() == from) && (message.getSeqNo() == seqNo)) {
            return message.getMessage();
        }
    }
    return "";
}