#ifndef MASTER_HPP
#define MASTER_HPP

#include "libraries.hpp"
#include "mapper.hpp"

class Master {
    public:
        Master(string input_dir, string output_dir, int nworkers, int nreduce);
        void map();

    private:
        string input_dir;
        string output_dir;
        int nworkers;
        int nreduce;
        std::thread *workers;
        vector<string> files;

        void beginMapReduce();
        void mapPhase();
        void reducePhase();
        void mergePhase();
        int countAndStoreFiles();
};

#endif // MASTER_HPP