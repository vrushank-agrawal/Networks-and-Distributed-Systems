#include "headers.hpp"

Reducer::Reducer(int id, string input_dir, string output_dir, int nworkers) {
    this->worker_id = id;
    this->input_dir = input_dir;
    this->output_dir = output_dir;
    this->nworkers = nworkers;
    // this->reduce();
}

void Reducer::reduce() {
    cout << "Reducer " << this->worker_id << " started" << endl;

    vector<string> files;
    for (int i = 0; i < this->nworkers; i++) {
        string filename = this->output_dir + "map.part-" + to_string(i) + "-" + to_string(this->worker_id) + ".txt";
        files.push_back(filename);
    }

    map<string, int> counts;
    for (string file : files) {
        ifstream input(file);
        string line;
        while (getline(input, line)) {
            string key = line.substr(0, line.find(','));
            counts[key] += 1;
        }
    }

    string filename = this->output_dir + "/reduce.part-" + to_string(this->worker_id) + ".txt";
    ofstream output(filename);
    for (auto it = counts.begin(); it != counts.end(); it++) {
        output << it->first << "," << it->second << "\n";
    }
    output.close();
}