# MapReduce Framework

Environment: C++20, Ubuntu 20.04<br>
Names: Vrushank Agrawal (vca4)

## High Level Details

Run ``make`` to generate the executable (`mapreduce`).

The code is separated into three main classes: Master, Mapper, and Reducer which are implemented in the master.cpp, mapper.cpp, and reducer.cpp files respectively. The headers for the classes are in the headers folder and are called through Makefile for compilation and linking.

## ./mapreduce

The main.cpp file is linked to mapreduce object file by the Makefile. The object takes the command of the form ``./mapreduce --input <input_file> --output <output_file> --nworkers <nworkers> --nreduce <nreduce>``. Here, the input_file is the directory of all the input files which are assumed to be of the format ``.txt``. output_file is the directory where the output will be stored, ``nworkers`` is the number of worker threads, and ``nreduce`` is the number of reducer tasks.

## Program Logic

The mapreduce program uses multithreading to implement the MapReduce framework. The program initially checks for the correct number and type of arguments. If the output directory already exists, it overwrites the directory. The program then creates a Master object which is the Master node in the MapReduce system that performs three main tasks:

- mapping
- reducing
- merging

### Master

The Master object identifies the input files and divides them into chunks for the worker threads. Essentially, the Master spawns ``nWorkers`` threads and equally assigns each thread a chunk of the input files in the indexed order. Doing this, the Master node only has to run the threads once and then wait for the threads to finish the [mapper](#mapper).

Once the mapper workers are done, the Master node assigns the [reducer](#reducer) tasks to the reducer threads. For the reducer tasks, the Master need not assign files to the threads as the mappers have already stored the temporary files in the output directory with appropriate naming.

Once the reducer tasks are done, the Master runs the final merger on its own thread. The Master node reads all the temporary reduce.part files and stores them in a map. Then it uses a comparator to sort the map by the values in descending order and in case of collisions of values, it sorts by keys in ascending order. Essentially, the final map is sorted by key and then value. Once the sorting is done, the Master node writes the final output to the output file ``output.txt`` within the output directory.

### Mapper

The Mapper object creates ``nReduce`` number of partition strings to store the intermediate key-value pairs based on the hash function of the key during partitioning. Then for each input file, the Mapper reads the file line by line and tokenizes the line into words.

When tokenizing, the Mapper checks if the word has a non-Latin character (non-English) in which case it ignores the word. For example, if the word is ``hello!`` then the Mapper will ignore the word. On the other hand, if the word is ``hello's`` then the Mapper will divide the word into two words ``hello`` and ``s`` using the function ``Mapper::symbolStrip``. Furthermore, while checking for non-Latin characters, the Mapper converts all the characters to lowercase to ensure that the keys are case-insensitive.

After tokenizing, it stores the key-value pairs in the partitioned strings based on the hash function of the key in the format ``key,1\n``. We use the polynomial rolling hash function to hash the keys which ensures that the keys are uniformly distributed across the partitions. Once the Mapper is done partitioning all the input files, it writes the partitioned strings to the output directory as temporary files.

### Reducer

The Reducer object reads the temporary files for its own id created by all the mappers. Doing this ensures that all the same keys are reduced by the same reducer because the keys are hashed to the same partition. The Reducer reads the temporary files and stores the key-value pairs in a map. It then reduces the values for each key by summing them up by one. Once the Reducer is done reducing all the temporary files, it writes the reduced key-value pairs in the format ``key,value\n`` to the output directory as temporary files.

## Issues faced

1. The first issue faced was accessing files in the input directory by the Mappers. The issue was resolved by creating a vector of strings that stored the file names in the input directory and then passing the file names to the Mapper threads as a pointer to the vector.
2. Another issue was faced in the polynomial rolling hash function. The issue was that by using signed integers, the hash function was returning negative values which were causing the hash function to return negative values and consequent segmentation faults. The issue was resolved by using size_t instead of int for the hash function.
3. The third issue was faced in the Merger phase where the keys were not being sorted correctly. The issue was resolved by using a custom comparator that sorted the keys by value in descending order and in case of collisions, sorted by key in ascending order.
