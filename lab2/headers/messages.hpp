#ifndef MESSAGE_CLASSES
#define MESSAGE_CLASSES

#include "libraries.hpp"

class RumorMessage {
    private:
        char* message;
        char* from;
        int seqNo;
        std::string chatText;
        void decomposeMessage();

    public:
        RumorMessage(char* message);

        /* Getters */
        std::string getChatText();
        char* getFrom();
        int getSeqNo();
};

class StatusMessage {
    private:
        std::map<int, int> status;
        std::vector<RumorMessage> chatLog;

    public:
        StatusMessage();
        StatusMessage(int port);
        void updateStatus(int port, int seqNo);

        /* Getters */
        std::map<int, int> getStatus();
        std::vector<RumorMessage> getChatLog();
};

#endif