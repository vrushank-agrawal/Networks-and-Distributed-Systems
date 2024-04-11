#include "headers.hpp"

Mapper::Mapper(int id, string input_dir, string output_dir, int start, int end, int nreduce, vector<string> *files) {
    this->worker_id = id;
    this->input_dir = input_dir;
    this->output_dir = output_dir;
    this->start_file = start;
    this->end_file = end;
    this->nreduce = nreduce;
    this->files = files;
    this->partitions = vector<string>(nreduce, "");
}

vector<string> Mapper::split(string line, char delimiter) {
    vector<string> words;
    stringstream ss(line);
    string word;
    while (getline(ss, word, delimiter)) {
        words.push_back(word);
    }
    return words;
}

int Mapper::partition(string key) {
    int hash = 0;
    for (char c : key) {
        hash = (hash * 31) + c;
    }
    return hash % this->nreduce;
}

void Mapper::createPartitionFiles() {
    for (int i = 0; i < this->nreduce; i++) {
        string filename = this->output_dir + "/map.part-" + to_string(this->worker_id) + "-" + to_string(i) + ".txt";
        ofstream output(filename);
        output << this->partitions[i];
        output.close();
    }
}

void Mapper::map() {
    for (int i = this->start_file; i < this->end_file; i++) {
        string file = this->files->at(i);
        ifstream input(file);
        string line;
        while (getline(input, line)) {
            vector<string> words = this->split(line, ' ');
            for (string word : words) {
                int part = this->partition(word);
                this->partitions[part] += (word + ",1\n");
            }
        }
    }
    this->createPartitionFiles();
}
