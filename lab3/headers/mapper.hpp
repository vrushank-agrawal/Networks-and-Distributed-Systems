#ifndef MAPPER_HPP
#define MAPPER_HPP

#include "libraries.hpp"

/**
 * @brief Mapper class
 * The Mapper class is responsible for processing the input
 * files and creating partition files for each reduce worker.
*/
class Mapper {
    public:
        /**
         * @brief Construct a new Mapper object
         *
         * @param id the worker id
         * @param input_dir the input directory
         * @param output_dir the output directory
         * @param start the first file index to process
         * @param end the last file index to process
         * @param nreduce the number of reduce partitions
         * @param files the list of files in the input_dir to process
         * @return Mapper the new Mapper object
        */
        Mapper(int id,
                string input,
                string output,
                int start,
                int end,
                int nreduce,
                vector<string> *files
        );

        /**
         * @brief Map the input files to reduce partitions and write to files
        */
        void map();

    private:
        int worker_id;              /**< worker id */
        string input_dir;           /**< input directory */
        string output_dir;          /**< output directory */
        int nreduce;                /**< number of reduce partitions */
        int start_file;             /**< first file index to process */
        int end_file;               /**< last file index to process */
        vector<string> *files;      /**< files in the input_dir to process */
        vector<string> partitions;  /**< partition strings for each reducer */

        /**
         * @brief Split a string by a delimiter
         *
         * @param line the string to split
         * @param delimiter the delimiter to split by
         * @return vector<string> the split string
        */
        vector<string> split(string line, char delimiter);

        /**
         * @brief Create partition files for each reduce partition
        */
        void createPartitionFiles();

        /**
         * @brief Hash a key (word) to a reduce partition
         *
         * @param key the key to partition
         * @return int the reduce partition number
        */
        int partition(string key);

        /**
         * @brief Check if a word is latin and convert it to lowercase
         *
         * @param w the word to check
         * @return true if the word is latin
         * @return false if the word is not latin
        */
        bool isLatin(string &word);
};

#endif // MAPPER_HPP