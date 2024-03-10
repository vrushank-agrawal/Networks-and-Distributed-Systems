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
        RumorMessage(int seq, char* message);

        /*****************************
         * Getters                   *
         * ***************************/
        std::string getMessage();
        char* getFrom();
        int getSeqNo();
        std::string getChatText();
};

class StatusMessage {
    private:
        std::map<int, int> status;
        std::vector<RumorMessage> chatLog;
        int maxSeqNo;

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
        int getMaxSeqNo();
        std::string getMessage(int seqNo);
};

#endif