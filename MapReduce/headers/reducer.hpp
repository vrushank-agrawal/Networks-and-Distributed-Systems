#ifndef REDUCER_HPP
#define REDUCER_HPP

#include "libraries.hpp"

/**
 * @brief Reducer class
 * The Reducer class is responsible for reducing the output
 * of the mappers and writing the unsorted output to a file.
*/
class Reducer {
    public:
        /**
         * @brief Construct a new Reducer object
         *
         * @param id the worker id
         * @param input_dir the input directory
         * @param output_dir the output directory
         * @param nworkers the number of worker threads
         * @return Reducer the new Reducer object
        */
        Reducer(int id,
                string input,
                string output,
                int nworkers
        );

        /**
         * @brief Reduce the output of the mappers
         *     1) Read the output of each mapper for this reducer
         *     2) Count the number of occurrences of each key
         *     3) Write the output to a file
        */
        void reduce();

    private:
        int worker_id;      /**< the worker id */
        string input_dir;   /**< the input directory */
        string output_dir;  /**< the output directory */
        int nworkers;       /**< the number of worker threads */
};

#endif // REDUCER_HPP