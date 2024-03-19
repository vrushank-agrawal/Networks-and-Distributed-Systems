#ifndef MESSAGE_CLASSES
#define MESSAGE_CLASSES

#include "libraries.hpp"

#define MAX_MESSAGES 1024

class RumorMessage {
    private:
        std::string message;
        std::string chatText;
        int from;
        int seqNo;

        void decomposeMessage();

    public:
        RumorMessage();
        RumorMessage(std::string message);

        /*****************************
         * Getters                   *
         * ***************************/
        std::string getMessage();
        std::string getChatText();
        int getFrom();
        int getSeqNo();
};

class StatusMessage {
    private:
        std::map<int, int> statusMap;
        std::vector<RumorMessage> chatLog;

    public:
        StatusMessage();
        StatusMessage(int port);

        /*****************************
         * Setters                   *
         * ***************************/
        void updateStatus(int port, int seqNo);
        void addMessageToLog(RumorMessage message, int seq);
        void updateMaxSeqNo(int seqNo);

        /*****************************
         * Getters                   *
         * ***************************/
        std::map<int, int> getStatus();
        std::vector<RumorMessage> getChatLog();
        std::string getMessage(int from, int seqNo);
        int getMaxSeqNo();
};

#endif