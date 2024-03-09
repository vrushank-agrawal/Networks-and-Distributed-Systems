#include "headers.hpp"

/***********************
 * RumorMessage class  *
 ***********************/

RumorMessage::RumorMessage(char* message) {
    this->message = message;
    // this->decomposeMessage();
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


/***********************
 * StatusMessage class *
 ***********************/

StatusMessage::StatusMessage() {}

StatusMessage::StatusMessage(int port) {
    this->status[port+1] = 0;
    this->status[port-1] = 0;
}

std::map<int, int> StatusMessage::getStatus() {
    return this->status;
}

std::vector<RumorMessage> StatusMessage::getChatLog() {
    return this->chatLog;
}

void StatusMessage::updateStatus(int port, int seqNo) {
    this->status[port] = seqNo;
}