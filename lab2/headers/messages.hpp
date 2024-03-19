#ifndef MESSAGE_CLASSES
#define MESSAGE_CLASSES

#include "libraries.hpp"

#define MAX_MESSAGES 1024

class RumorMessage {
    private:
        std::string message;
        std::string from;
        int seqNo;
        std::string chatText;

        void decomposeMessage();

    public:
        RumorMessage();
        RumorMessage(int seq, std::string message);
        bool operator==(const RumorMessage& other) const;

        /*****************************
         * Getters                   *
         * ***************************/
        std::string getMessage();
        std::string getFrom();
        int getSeqNo();
        std::string getChatText();
};

class StatusMessage {
    private:
        std::map<int, int> statusMap;
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