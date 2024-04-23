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

void Mapper::createPartitionFiles() {
    for (int i = 0; i < this->nreduce; i++) {
        string filename = this->output_dir + "/map.part-" + to_string(this->worker_id) + "-" + to_string(i) + ".txt";
        ofstream output(filename);
        output << this->partitions[i];
        output.close();
    }
}

int Mapper::partition(string key) {
    size_t hash = 0;
    for (char c : key) {
        hash = (hash * 31) + c;
    }
    return int(hash % this->nreduce);
}

bool Mapper::isLatin(string &w) {
    string new_w = "";
    for (char c : w) {
        if (!isalpha(c)) {
            return false;
        }
        if (isupper(c)) {
            new_w += tolower(c);
        } else {
            new_w += c;
        }
    }
    w = new_w;
    return true;
}

void Mapper::symbolStrip(vector<string> &words) {
    vector<string> strip_commas;
    vector<string> strip_apostrophes;

    /* Strip commas */
    for (string word : words) {
        if (word.find(',') != string::npos) {
            vector<string> temp = this->split(word, ',');
            for (string t : temp) strip_commas.push_back(t);
        } else if (word.find('\'') != string::npos) {
            strip_commas.push_back(word);
        }
    }

    /* Strip apostrophes */
    for (string word : strip_commas) {
        if (word.find('\'') != string::npos) {
            vector<string> temp = this->split(word, '\'');
            for (string t : temp) strip_apostrophes.push_back(t);
        } else {
            strip_apostrophes.push_back(word);
        }
    }

    /* append new words to words */
    for (string word : strip_apostrophes) {
        if (word == " " ||
            word == "" ||
            word == "," ||
            word == "\'"
        ) continue;

        words.push_back(word);
    }
}

void Mapper::map() {
    for (int i = this->start_file; i < this->end_file; i++) {
        cout << "Mapping file: " << this->files->at(i) << endl;
        string file = this->files->at(i);
        ifstream input(file);
        string line;
        while (getline(input, line)) {
            vector<string> words = this->split(line, ' ');
            this->symbolStrip(words);
            for (string word : words) {
                if (!this->isLatin(word)) continue;
                int part = this->partition(word);
                this->partitions[part] += (word + ",1\n");
            }
        }
    }
    this->createPartitionFiles();
}
