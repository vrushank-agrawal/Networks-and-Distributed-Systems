#ifndef MAPPER_HPP
#define MAPPER_HPP

#include "libraries.hpp"

class Mapper {
    public:
        Mapper(int id, string input, string output, int start, int end, int nreduce, vector<string> *files);
        void map();

    private:
        int worker_id;
        string input_dir;
        string output_dir;
        int nreduce;
        int start_file;
        int end_file;
        vector<string> *files;
        vector<string> partitions;

        vector<string> split(string line, char delimiter);
        void createPartitionFiles();
        int partition(string key);
        bool isLatin(string &word);
};

#endif // MAPPER_HPP