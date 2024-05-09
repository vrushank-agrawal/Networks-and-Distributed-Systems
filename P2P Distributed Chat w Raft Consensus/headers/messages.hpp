#ifndef MESSAGE_CLASSES
#define MESSAGE_CLASSES

#include "libraries.hpp"

#define MAX_MESSAGES 1024


class LogEntry {
    public:
        int term;
        int messageId;
        std::string message;
        std::string isCommitted;

        LogEntry(int term, int messageId, std::string message, std::string isCommitted = "true") {
            this->term = term;
            this->messageId = messageId;
            this->message = message;
            this->isCommitted = isCommitted;
        }

};

class Log {
    private:
        std::vector<LogEntry> entries;
    public:
    // Append a new entry to the log
    void appendEntry(LogEntry entry) {
        std::cout << "Printing log entries before push_back: " << std::endl;
        this->printLogEntries();
        entries.push_back(entry);
        std::cout << "Appended entry " << entry.term << "," << entry.messageId << "," << entry.message << "," << entry.isCommitted << std::endl;
        std::cout << "Printing log entries: " << std::endl;
        this->printLogEntries();
    }

    std::vector<LogEntry> getEntries() {
        return entries;
    }

    std::string getEntriesAsStringMessage() {
        std::string message = "";

        for (const auto& entry : this->entries) {
            message += std::to_string(entry.term) + "," + std::to_string(entry.messageId) + "," + entry.message + "," + entry.isCommitted  + ";";
        }
        message+= "\n";
        return message;
    }

    // std::vector<LogEntry> getLogEntriesFromStringMessage(const std::string& message) {
    //     std::vector<LogEntry> entries;
    //     std::stringstream ss(message);
    //     std::string item;

    //     // FOR LOOPS ONLY!!!!
    //     for (int i = 0; i < message.size(); i++) {
    //         if (message[i] == ';') {
    //             std::string b = (entries[i].isCommitted) ? "1" : "0";
    //             message += std::to_string(entries[i].term) + "," + std::to_string(entries[i].messageId) + "," + entries[i].message + "," + b + ";";
    //         }
    //     }

    //     return entries;
    // }

    std::vector<LogEntry> getLogEntriesFromStringMessage(const std::string& serializedLog) {
        std::vector<LogEntry> entries;
        std::stringstream ss(serializedLog);
        std::string item;

        // Split by semicolon to get individual log entries
        while (getline(ss, item, ';')) {
            if (item.empty()) {
                continue;
            }
            std::stringstream entryStream(item);
            std::string part;
            std::vector<std::string> parts;

            // Split by comma to get term, messageId, message, and isCommitted
            while (getline(entryStream, part, ',')) {
                parts.push_back(part);
            }

            if (parts.size() == 4) {
                int term = std::stoi(parts[0]);
                int messageId = std::stoi(parts[1]);
                std::string message = parts[2];
                std::string isCommitted = parts[3];
                std::cout << "inside getLogEntriesFromStringMessage, item: " << item << std::endl;
                std::cout << "term: " << term << ", messageId: " << messageId << ", message: " << message << ", isCommitted: " << isCommitted << std::endl;

                entries.emplace_back(term, messageId, message, isCommitted);
            } else {
                std::cout << "we did not get 4 parts for: " << item << std::endl;
            }
        }

        return entries;
    }

    void printLogEntries() const {
        for (const auto& entry : entries) {
            std::cout << entry.term << "," << entry.messageId << "," << entry.message << "," << entry.isCommitted << std::endl;
        }
    }

    void setEntries(const std::vector<LogEntry>& entries) {
        this->entries = entries;
    }

    std::vector<LogEntry> getCommittedEntries() const {
        std::cout << "Printing log entries before getCommittedEntries: " << std::endl;
        this->printLogEntries();
        std::vector<LogEntry> committedEntries;
        for (size_t i = 0; i < entries.size(); i++) {
            if (entries[i].isCommitted == "true") {
                committedEntries.push_back(entries[i]);
            }
        }
        std::cout << "Printing log entries after getCommittedEntries: " << std::endl;
        this->printLogEntries();
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