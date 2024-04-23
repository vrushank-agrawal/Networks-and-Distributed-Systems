#ifndef MASTER_HPP
#define MASTER_HPP

#include "libraries.hpp"
#include "mapper.hpp"

class Master {
    public:
        /**
         * @brief Construct a new Master object
         *      Begin the map reduce process
         *
         * @param input_dir the input directory
         * @param output_dir the output directory
         * @param nworkers the number of worker threads
         * @param nreduce the number of reduce threads
         */
        Master(string input_dir,
                string output_dir,
                int nworkers,
                int nreduce
        );

    private:
        string input_dir;       /**< the input directory */
        string output_dir;      /**< the output directory */
        int nworkers;           /**< the number of worker threads */
        int nreduce;            /**< the number of reduce threads */
        std::thread *workers;   /**< the worker threads */
        vector<string> files;   /**< the files in the input directory */

        /**
         * @brief Start the map phase
         *      1) Count the number of files in the input directory
         *      2) Create a mapper for each thread
         *      3) Allocate an equal number of files to each mapper
         *      4) Start a new thread for each mapper
         *      5) Wait for all workers to finish
         */
        void mapPhase();

        /**
         * @brief Start the reduce phase
         *      1) Create a reducer for each thread
         *      2) Start a new thread for each reducer
         *      3) Wait for all workers to finish
         */
        void reducePhase();

        /**
         * @brief Start the merge phase
         *      1) Read the output of the reduce phase
         *      2) Sort the output by value and then key
         *      3) Write the output to a file
         */
        void mergePhase();

        /**
         * @brief Run the map reduce process
         *     1) map
         *     2) reduce
         *     3) merge
         */
        void beginMapReduce();

        /**
         * @brief Count the number of files in the input directory
         *      Store the files in a vector
         *
         * @return int the number of files
         */
        int countAndStoreFiles();
};

/**
 * @brief Sort the output of the reduce phase and write to a file
 *     1) Sort by value
 *     2) If values are equal, sort by key
 *
 * @param a the first pair
 * @param b the second pair
 * @return true if a.value > b.value
 */
bool sortByValue(const pair<string, int> &a,
                const pair<string, int> &b);

#endif // MASTER_HPP