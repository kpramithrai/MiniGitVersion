#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include<vector>
#include<algorithm>
#include<chrono>

using namespace std;
namespace fs = filesystem;

// Computes the SHA-1 hash of a string
string compute_sha1(const string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    
    stringstream ss;
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << hex << setw(2) << setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// NEW: Converts a 40-character hex string into 20 raw bytes
string hex_to_bytes(const string& hex_str) {
    string bytes;
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        bytes.push_back(static_cast<char>(stoi(hex_str.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

// NEW: Converts 20 raw SHA-1 bytes into a 40-character lowercase hex string.
// This is the reverse of hex_to_bytes() and is needed because tree entries
// store the referenced object's hash as raw bytes, but read_object()/cat_file()
// index the object database using the 40-character hex form.
string bytes_to_hex(const string& bytes) {
    stringstream ss;
    for (unsigned char c : bytes) { // char -> unsigned char keeps the value 0-255 correct
        ss << hex << setw(2) << setfill('0') << static_cast<int>(c);
    }
    return ss.str();
}

// NEW: A universal function to hash and write ANY type of Git object
string write_object(const string& type, const string& content) {
    string header = type + " " + to_string(content.size()) + '\0';
    string store_data = header + content;
    string sha1_hash = compute_sha1(store_data);

    string dir_name = sha1_hash.substr(0, 2);
    string file_name = sha1_hash.substr(2);
    
    fs::path object_dir = fs::path(".mygit/objects") / dir_name;
    fs::path object_path = object_dir / file_name;

    fs::create_directories(object_dir);
    
    ofstream out_file(object_path, ios::binary);
    if (out_file.is_open()) {
        out_file << store_data;
        out_file.close();
    } else {
        cerr << "Error: Could not write object to database.\n";
    }
    return sha1_hash;
}

void init_mygit() {
    try {
        fs::create_directories(".mygit/objects");
        fs::create_directories(".mygit/refs/heads");

        ofstream head_file(".mygit/HEAD");
        if (head_file.is_open()) {
            head_file << "ref: refs/heads/master\n";
            head_file.close();
        } else {
            cerr << "Error: Could not create HEAD file.\n";
        }
    } catch (const fs::filesystem_error& e) {
        cerr << "Error creating directories: " << e.what() << '\n';
    }
}


// UPDATED: Now incredibly clean thanks to our refactor!
string hash_object(const string& filepath) {
    ifstream file(filepath, ios::binary);   //Does this open the file in binary and content has binary data??
    if (!file.is_open()) {
        cerr << "Fatal: Cannot open file " << filepath << '\n';
        return "";
    }
    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    return write_object("blob", content);
}

// NEW: Helper to read an object's raw content into memory without printing it
string read_object(const string& sha1_hash) {
    if (sha1_hash.length() != 40) return "";

    string dir_name = sha1_hash.substr(0, 2);
    string file_name = sha1_hash.substr(2);
    fs::path object_path = fs::path(".mygit/objects") / dir_name / file_name;

    if (!fs::exists(object_path)) return "";

    ifstream file(object_path, ios::binary);
    if (!file.is_open()) return "";

    string raw_data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    size_t null_byte_pos = raw_data.find('\0');
    if (null_byte_pos != string::npos) {
        return raw_data.substr(null_byte_pos + 1);
    }
    return "";
}

// NEW: Reads a blob from the object database and prints its contents
void cat_file(const string& sha1_hash) {
    // Hashes must be exactly 40 characters long
    if (sha1_hash.length() != 40) {
        cerr << "Fatal: Not a valid SHA-1 hash\n";
        return;
    }

    // Split the hash to locate the file path
    string dir_name = sha1_hash.substr(0, 2);
    string file_name = sha1_hash.substr(2);
    fs::path object_path = fs::path(".mygit/objects") / dir_name / file_name;

    if (!fs::exists(object_path)) {
        cerr << "Fatal: Not a valid object name " << sha1_hash << '\n';
        return;
    }

    // Open the file in binary mode
    ifstream file(object_path, ios::binary);
    if (!file.is_open()) {
        cerr << "Fatal: Cannot open object file\n";
        return;
    }

    // Read the entire file into memory
    string raw_data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    file.close();

    // The data format is "blob <size>\0<content>"
    // Find the null byte '\0' that separates the header from the actual text
    size_t null_byte_pos = raw_data.find('\0');
    if (null_byte_pos != string::npos) {
        // Print everything immediately after the null byte
        cout << raw_data.substr(null_byte_pos + 1);
    } else {
        cerr << "Fatal: Corrupted object file (no header found)\n";
    }
}

struct TreeEntry {
    string mode;
    string filename;
    string sha1;
};

// NEW: Parses raw binary tree content ("<mode> <filename>\0<20 raw hash bytes>" repeated)
// into a list of TreeEntry structs. This must NOT use C-string functions (like strlen or
// c_str()-based scanning) because the 20 raw hash bytes can legitimately contain 0x00,
// which would fool anything that treats the buffer as null-terminated.
vector<TreeEntry> parse_tree_entries(const string& tree_content) {
    vector<TreeEntry> entries;
    size_t pos = 0;
    const size_t len = tree_content.size();

    while (pos < len) {
        size_t space_pos = tree_content.find(' ', pos);
        if (space_pos == string::npos) break; // malformed/truncated tree

        string mode = tree_content.substr(pos, space_pos - pos);

        size_t null_pos = tree_content.find('\0', space_pos + 1);
        if (null_pos == string::npos) break; // malformed/truncated tree

        string filename = tree_content.substr(space_pos + 1, null_pos - space_pos - 1);

        if (null_pos + 1 + 20 > len) break; // truncated: fewer than 20 bytes left for the hash

        string raw_hash = tree_content.substr(null_pos + 1, 20);
        entries.push_back({mode, filename, bytes_to_hex(raw_hash)});

        pos = null_pos + 1 + 20; // advance past this entry's fixed-size hash
    }

    return entries;
}



// CHANGED: Now returns string instead of void
string write_tree() {
    vector<TreeEntry> entries;

    for (const auto& entry : fs::directory_iterator(".")) {
        string filename = entry.path().filename().string();
        
        if (filename == ".mygit" || filename == "Makefile") {
            continue; 
        }

        if (entry.is_regular_file()) {
            string file_hash = hash_object(entry.path().string());
            if (!file_hash.empty()) {
                entries.push_back({"100644", filename, file_hash});
            }
        }
    }

    sort(entries.begin(), entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        return a.filename < b.filename;
    });

    string tree_content = "";
    for (const auto& entry : entries) {
        tree_content += entry.mode + " " + entry.filename + '\0' + hex_to_bytes(entry.sha1);
    }

    string tree_hash = write_object("tree", tree_content);
    return tree_hash; // NEW: Return the hash so the commit function can use it!
}

// NEW: Reads HEAD and resolves it to the current branch's ref path, e.g. "refs/heads/master".
// Both commit() and print_log() need this, but print_log() already had its own inline copy
// of this logic; commit() now uses this shared helper so the branch is never hardcoded.
string get_current_branch_ref() {
    ifstream head_file(".mygit/HEAD");
    if (!head_file.is_open()) {
        return "";
    }

    string head_ref;
    getline(head_file, head_ref);
    head_file.close();

    if (head_ref.find("ref: ") == 0) {
        head_ref = head_ref.substr(5);
    }
    // Trim any trailing CR/whitespace so the path we build is clean
    while (!head_ref.empty() && (head_ref.back() == '\r' || head_ref.back() == '\n' || head_ref.back() == ' ')) {
        head_ref.pop_back();
    }
    return head_ref;
}

void commit(const string& message) {
    // 1. Resolve the current branch through HEAD instead of hardcoding "master"
    string branch_ref = get_current_branch_ref();
    if (branch_ref.empty()) {
        cerr << "Fatal: Not a mygit repository (bad or missing HEAD).\n";
        return;
    }
    string branch_path = ".mygit/" + branch_ref;

    // 2. Read the branch's current tip, if any -- this becomes the new commit's parent
    string parent_hash;
    ifstream branch_in(branch_path);
    if (branch_in.is_open()) {
        getline(branch_in, parent_hash);
        branch_in.close();
    }

    // 3. Generate the tree for the current directory state
    string tree_hash = write_tree();
    if (tree_hash.empty()) {
        cerr << "Fatal: Failed to write tree.\n";
        return;
    }

    // 4. Get the current Unix timestamp
    auto now = chrono::system_clock::now();
    time_t current_time = chrono::system_clock::to_time_t(now);

    // 5. Format the commit content -- include a parent line only if a previous commit exists
    string commit_content = "tree " + tree_hash + '\n';
    if (!parent_hash.empty()) {
        commit_content += "parent " + parent_hash + '\n';
    }
    commit_content += "author MyGit User <user@mygit.local> " + to_string(current_time) + " +0000\n";
    commit_content += "committer MyGit User <user@mygit.local> " + to_string(current_time) + " +0000\n\n";
    commit_content += message + '\n';

    // 6. Save the commit object using our universal function
    string commit_hash = write_object("commit", commit_content);

    // 7. Only now, after the commit object safely exists, update the branch reference
    ofstream ref_file(branch_path);
    if (ref_file.is_open()) {
        ref_file << commit_hash << '\n';
        ref_file.close();

        // Derive a short display name (e.g. "master") from "refs/heads/master"
        string branch_name = branch_ref.substr(branch_ref.find_last_of('/') + 1);

        if (parent_hash.empty()) {
            cout << "[" << branch_name << " (root-commit) " << commit_hash.substr(0, 7) << "] " << message << '\n';
        } else {
            cout << "[" << branch_name << " " << commit_hash.substr(0, 7) << "] " << message << '\n';
        }
    } else {
        cerr << "Fatal: Could not update branch reference.\n";
    }
}

void print_log() {
    // 1. Read the HEAD file to find the current branch
    ifstream head_file(".mygit/HEAD");
    if (!head_file.is_open()) {
        cerr << "Fatal: Not a mygit repository.\n";
        return;
    }
    
    string head_ref;
    getline(head_file, head_ref);
    head_file.close();

    // Parse out the "ref: " part to get the actual path
    if (head_ref.find("ref: ") == 0) {
        head_ref = head_ref.substr(5);
    }

    // 2. Read the branch file to get the latest commit hash
    ifstream branch_file(".mygit/" + head_ref);
    if (!branch_file.is_open()) {
        cerr << "Fatal: No commits yet.\n";
        return;
    }

    string current_commit_hash;
    getline(branch_file, current_commit_hash);
    branch_file.close();

    // 3. Traverse the commit history loop
    while (!current_commit_hash.empty()) {
        string commit_content = read_object(current_commit_hash);
        if (commit_content.empty()) {
            cerr << "Fatal: Could not read commit object.\n";
            break;
        }

        cout << "commit " << current_commit_hash << '\n';

        // Very basic parsing to format the log nicely
        istringstream stream(commit_content);
        string line;
        string parent_hash = "";

        while (getline(stream, line)) {
            if (line.empty()) {
                // An empty line in a commit object means the message is starting
                string message;
                getline(stream, message);
                cout << "\n    " << message << "\n\n";
                break;
            } else if (line.find("author ") == 0) {
                cout << "Author: " << line.substr(7) << '\n';
            } else if (line.find("parent ") == 0) {
                // If this commit has a parent, save it for the next loop iteration
                parent_hash = line.substr(7);
            }
        }
        
        // Move to the parent commit (if there is no parent, this ends the loop)
        current_commit_hash = parent_hash;
    }
}

// NEW: Restores a single file's content from a historical commit into the working directory.
// Follows the safe order required by the spec: fully validate and read the commit, tree,
// matching entry, and blob BEFORE touching the destination file on disk.
void checkout(const string& commit_hash, const string& filename) {
    if (!fs::exists(".mygit")) {
        cerr << "Fatal: Not a mygit repository.\n";
        return;
    }

    // Step 1: validate and read the commit object
    string commit_content = read_object(commit_hash);
    if (commit_content.empty()) {
        cerr << "Fatal: Invalid commit hash or missing commit object.\n";
        return;
    }

    // Extract the "tree <hash>" line
    string tree_hash;
    {
        istringstream stream(commit_content);
        string line;
        while (getline(stream, line)) {
            if (line.find("tree ") == 0) {
                tree_hash = line.substr(5);
                break;
            }
        }
    }
    if (tree_hash.empty()) {
        cerr << "Fatal: Commit object is missing a tree line.\n";
        return;
    }

    // Step 2: read the binary tree object
    string tree_content = read_object(tree_hash);
    if (tree_content.empty()) {
        cerr << "Fatal: Missing or unreadable tree object.\n";
        return;
    }

    // Step 3 & 4: parse the tree entries in a binary-safe way
    vector<TreeEntry> entries = parse_tree_entries(tree_content);
    if (entries.empty()) {
        cerr << "Fatal: Tree object is empty or malformed.\n";
        return;
    }

    // Step 5: find the requested file
    auto it = find_if(entries.begin(), entries.end(), [&](const TreeEntry& e) {
        return e.filename == filename;
    });
    if (it == entries.end()) {
        cerr << "Fatal: File '" << filename << "' not found in commit " << commit_hash << ".\n";
        return;
    }

    // Step 6: read the historical blob
    string blob_content = read_object(it->sha1);
    if (blob_content.empty()) {
        cerr << "Fatal: Missing or unreadable blob object for '" << filename << "'.\n";
        return;
    }

    // Step 7: only now overwrite the working file
    ofstream out_file(filename, ios::binary | ios::trunc);
    if (!out_file.is_open()) {
        cerr << "Fatal: Could not open '" << filename << "' for writing.\n";
        return;
    }
    out_file << blob_content;
    out_file.close();

    cout << "Restored '" << filename << "' from commit " << commit_hash.substr(0, 7) << ".\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: ./mygit <command>\n";
        return 1;
    }

    string command = argv[1];

    if (command == "init") {
        init_mygit();
    } else if (command == "hash-object") {
        if (argc < 3) {
            cerr << "Usage: ./mygit hash-object <file>\n";
            return 1;
        }
        // NEW: Print the returned hash to the terminal
        string hash = hash_object(argv[2]);
        if (!hash.empty()) {
            cout << hash << '\n';
        }
    } else if (command == "cat-file") {
        // NEW: Route for the cat-file command
        if (argc < 3) {
            cerr << "Usage: ./mygit cat-file <hash>\n";
            return 1;
        }
        cat_file(argv[2]);
    } else if (command == "write-tree") {
        cout << write_tree() << '\n';
    } else if (command == "commit") {
        if (argc < 4 || string(argv[2]) != "-m") {
            cerr << "Usage: ./mygit commit -m \"<message>\"\n";
            return 1;
        }
        commit(argv[3]);
    } else if (command == "log") {
        // NEW: Route for the log command
        print_log();
    } else if (command == "checkout") {
        // NEW: Route for the checkout command
        if (argc < 4) {
            cerr << "Usage: ./mygit checkout <commit-hash> <filename>\n";
            return 1;
        }
        checkout(argv[2], argv[3]);
    } else {
        cerr << "Unknown command: " << command << '\n';
    }

    return 0;
}