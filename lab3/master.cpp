#include "headers.hpp"

Master::Master(string input_dir, string output_dir, int nworkers, int nreduce) {
    this->input_dir = input_dir;
    this->output_dir = output_dir;
    this->nworkers = nworkers;
    this->nreduce = nreduce;
    this->beginMapReduce();
}

void Master::mapPhase() {
    cout << "\nMap phase started" << endl;

    // count number of files in input directory
    int nFiles = this->countAndStoreFiles();

    // create a vector of workers
    this->workers = new std::thread[this->nworkers];

    // create a mapper for each thread and allocate an equal number of files to each mapper
    int filesPerWorker = nFiles / this->nworkers;
    int remainingFiles = nFiles % this->nworkers;
    int start = 0;

    for (int i = 0; i < this->nworkers; i++) {
        int end = start + filesPerWorker;
        if (i < remainingFiles) {
            end++;
        }

        // start a new thread for each mapper
        cout << "Worker " << i << " will process files " << start << " to " << end << endl;
        Mapper* mapper = new Mapper(i, this->input_dir, this->output_dir, start, end, this->nreduce, &this->files);
        this->workers[i] = std::thread(&Mapper::map, mapper);

        start = end;
    }

    // wait for all workers to finish
    for (int i = 0; i < this->nworkers; i++) {
        this->workers[i].join();
    }

    cout << "Map phase complete\n" << endl;
}

void Master::reducePhase() {
    cout << "Reduce phase started" << endl;

    // create a vector of workers
    this->workers = new std::thread[this->nreduce];

    // create a reducer for each thread
    for (int i = 0; i < this->nreduce; i++) {
        Reducer* reducer = new Reducer(i, this->input_dir, this->output_dir, this->nworkers);
        this->workers[i] = std::thread(&Reducer::reduce, reducer);
    }

    // wait for all workers to finish
    for (int i = 0; i < this->nreduce; i++) {
        this->workers[i].join();
    }

    cout << "Reduce phase complete\n" << endl;
}

void Master::mergePhase() {
    cout << "Merge phase started" << endl;

    map<string, int> counts;
    for (int i = 0; i < this->nreduce; i++) {
        string filename = this->output_dir + "/reduce.part-" + to_string(i) + ".txt";
        ifstream input(filename);
        string line;
        while (getline(input, line)) {
            string key = line.substr(0, line.find(','));
            int value = stoi(line.substr(line.find(',') + 1));
            counts[key] += value;
        }
    }

    vector<pair<string, int>> sorted(counts.begin(), counts.end());
    sort(sorted.begin(), sorted.end(), sortByValue);

    string filename = this->output_dir + "/output.txt";
    ofstream output(filename);
    for (auto it = sorted.begin(); it != sorted.end(); it++) {
        output << it->first << "," << it->second << "\n";
    }
    output.close();

    cout << "Merge phase complete" << endl;
}

void Master::beginMapReduce() {
    /* Start the map phase */
    this->mapPhase();

    /* Start the reduce phase */
    this->reducePhase();

    /* Start the merge phase */
    this->mergePhase();
}

int Master::countAndStoreFiles () {
    int count = 0;
    for (const auto & entry : filesystem::directory_iterator(this->input_dir)) {
        if (entry.path().extension() == ".txt") {
            this->files.push_back(entry.path());
            count++;
        }
    }
    return count;
}

bool sortByValue(const pair<string, int> &a, const pair<string, int> &b) {
    return (a.second == b.second) ? (a.first < b.first) : (a.second > b.second);
}