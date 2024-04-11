#ifndef REDUCER_HPP
#define REDUCER_HPP

#include "libraries.hpp"

class Reducer {
    public:
        Reducer(int id, string input, string output, int nworkers);
        void reduce();

    private:
        int worker_id;
        string input_dir;
        string output_dir;
        int nworkers;
};

#endif // REDUCER_HPP