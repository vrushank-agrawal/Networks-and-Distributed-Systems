#include "headers.hpp"

/***********************
 * RumorMessage class  *
 ***********************/

RumorMessage::RumorMessage(int seq, char* message) {
    this->seqNo = seq;
    this->message = message;
    // this->decomposeMessage();
}

void RumorMessage::decomposeMessage() {

}

std::string RumorMessage::getChatText() {
    return this->chatText;
}

char* RumorMessage::getFrom() {
    return this->from;
}

int RumorMessage::getSeqNo() {
    return this->seqNo;
}

std::string RumorMessage::getMessage() {
    return this->message;
}


/***********************
 * StatusMessage class *
 ***********************/

StatusMessage::StatusMessage() {}

StatusMessage::StatusMessage(int port) {
    this->status[port+1] = 0;
    this->status[port-1] = 0;
    this->maxSeqNo = 0;
}

void StatusMessage::updateStatus(int port, int seqNo) {
    this->status[port] = seqNo;
}

void StatusMessage::addMessageToLog(RumorMessage message, int seq) {
    std::cout << "Adding message " << seq << " to chat log" << std::endl;
    this->chatLog.push_back(message);
    this->updateMaxSeqNo(seq);
}

void StatusMessage::updateMaxSeqNo(int seqNo) {
    this->maxSeqNo = seqNo;
}

std::map<int, int> StatusMessage::getStatus() {
    return this->status;
}

std::vector<RumorMessage> StatusMessage::getChatLog() {
    return this->chatLog;
}

int StatusMessage::getMaxSeqNo() {
    return this->maxSeqNo;
}

std::string StatusMessage::getMessage(int seqNo) {
    for (RumorMessage message : this->chatLog) {
        if (message.getSeqNo() == seqNo) {
            return message.getChatText();
        }
    }

    /* Message not found */
    std::cerr << "Error: Message "<< seqNo << " not found in chatLog with maxSeqNo " << this->maxSeqNo << std::endl;
    return "";
}