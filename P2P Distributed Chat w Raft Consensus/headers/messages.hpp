#ifndef MESSAGE_CLASSES
#define MESSAGE_CLASSES

#include "libraries.hpp"

#define MAX_MESSAGES 1024


class LogEntry {
    public:
        int term;
        int messageId;
        std::string message;
        bool isCommitted;

        LogEntry(int term, int messageId, std::string message, bool isCommitted = false)
            : term(term), messageId(messageId), message(std::move(message)) {}
};

class Log {
    private:
        std::vector<LogEntry> entries;
    public:
    // Append a new entry to the log
    void appendEntry(const LogEntry& entry) {
        entries.push_back(entry);
    }

    std::vector<LogEntry> getEntries() const {
        return entries;
    }

    std::vector<LogEntry> getCommittedEntries() const {
        std::vector<LogEntry> committedEntries;
        for (size_t i = 0; i < entries.size(); i++) {
            if (entries[i].isCommitted) {
                committedEntries.push_back(entries[i]);
            }
        }
        return committedEntries;
    }

    // Get a specific log entry by index
    const LogEntry& getEntry(size_t index) const {
        // Ensure the index is valid
        if (index >= entries.size()) {
            throw std::out_of_range("Log index out of range");
        }
        return entries[index];
    }

    // Get a specific log entry by index (non-const version)
    LogEntry& getEntry(size_t index) {
        // Ensure the index is valid
        if (index >= entries.size()) {
            throw std::out_of_range("Log index out of range");
        }
        return entries[index];
    }

    // Get the latest term
    int getLastTerm() const {
        if (entries.empty()) {
            return 0;  // Return 0 if log is empty
        }
        return entries.back().term;
    }

    // Get the number of entries in the log
    size_t size() const {
        return entries.size();
    }

};

class RumorMessage {
    private:
        std::string message;
        std::string chatText;
        int from;
        int seqNo;

        /**
         * @brief Decompose the message into from, chatText and seqNo
         *      Message format: "from:___,chatText:___,seqNo:___"
        */
        void decomposeMessage();

    public:
        RumorMessage();

        /**
         * @brief Construct a new Rumor Message:: Rumor Message object
         * @param message Message received
        */
        RumorMessage(std::string message);

        /*****************************
         * Getters                   *
         * ***************************/

        /**
         * @brief Get the chatText from the message
        */
        std::string getChatText();

        /**
         * @brief Get the message
        */
        std::string getMessage();

        /**
         * @brief Get the sender of the message
        */
        int getFrom();

        /**
         * @brief Get the sequence number of the message
        */
        int getSeqNo();
};

class StatusMessage {
    private:
        std::map<int, int> statusMap;
        std::vector<RumorMessage> chatLog;

    public:
        StatusMessage();

        /**
         * @brief Construct a new Status Message:: Status Message object
         * @param port Port number of the server
        */
        StatusMessage(int port);

        /*****************************
         * Setters                   *
         * ***************************/

        /**
         * @brief Update the status of max messages at the other server
         * @param port Port number of the other server
         * @param seqNo Sequence number of the message
        */
        void updateStatus(int port, int seqNo);

        /**
         * @brief Add a message to the global chat log of current server
         * @param message Message to be added
         * @param seq Sequence number of the message (also the max sequence number)
        */
        void addMessageToLog(RumorMessage message, int seq);

        /*****************************
         * Getters                   *
         * ***************************/

        /**
         * @brief Get the status of the other server
        */
        std::map<int, int> getStatus();

        /**
         * @brief Get the chat log of the server until maxSeqNo elements
        */
        std::vector<RumorMessage> getChatLog();

        /**
         * @brief Get a specific message from the chat log
         * @param seqNo Sequence number of the message to retrieve
        */
        std::string getMessage(int from, int seqNo);
};

#endif