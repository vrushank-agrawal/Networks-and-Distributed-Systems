#include "headers.hpp"

vector<string> getParams(int argc, char* argv[]) {
    vector<string> args{4};

    vector<string> flags = {
        "--input",
        "--output",
        "--nworkers",
        "--nreduce",
    };

    string usage = "Usage: ./mapreduce --input <input_file> --output <output_file> --nworkers <nworkers> --nreduce <nreduce>";

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg.substr(0, 2) != "--") {
            cout << "Invalid flag: " << arg << endl;
            cout << usage << endl;
            exit(1);
        }

        auto it = find(flags.begin(), flags.end(), arg);
        if (it == flags.end()) {
            cout << "Invalid flag: " << arg << endl;
            cout << usage << endl;
            exit(1);
        }

        int index = distance(flags.begin(), it);
        i++;
        if (i >= argc) {
            cout << "Missing value for flag: " << arg << endl;
            cout << usage << endl;
            exit(1);
        }

        args[index] = argv[i];
    }

    return args;
}

bool isDir(string path) {
    return filesystem::is_directory(path);
}

bool makeDir(string path) {
    if (isDir(path)) {
        cout << "Overwriting " << path << endl;
    }

    try {
        filesystem::create_directory(path);
        cout << "Created directory: " << path << endl;
        return true;
    } catch (const filesystem::filesystem_error& e) {
        cerr << e.what() << endl;
        return false;
    }
}

int main (int argc, char*argv[]) {

    vector<string> args = getParams(argc, argv);

    string input_dir = args[0];
    string output_dir = args[1];
    int nworkers = stoi(args[2]);
    int nreduce = stoi(args[3]);

    if (!isDir(input_dir)) {
        cout << "Invalid input directory: " << input_dir << endl;
        return 1;
    }

    if (!makeDir(output_dir)) {
        cout << "Could not create output directory: " << output_dir << endl;
        return 1;
    }

    return 0;
}