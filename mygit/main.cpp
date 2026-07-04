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

namespace fs = std::filesystem;

// Computes the SHA-1 hash of a string
std::string compute_sha1(const std::string& data) {
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA1(reinterpret_cast<const unsigned char*>(data.c_str()), data.size(), hash);
    
    std::stringstream ss;
    for(int i = 0; i < SHA_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

// NEW: Converts a 40-character hex string into 20 raw bytes
std::string hex_to_bytes(const std::string& hex_str) {
    std::string bytes;
    for (size_t i = 0; i < hex_str.length(); i += 2) {
        bytes.push_back(static_cast<char>(std::stoi(hex_str.substr(i, 2), nullptr, 16)));
    }
    return bytes;
}

// NEW: A universal function to hash and write ANY type of Git object
std::string write_object(const std::string& type, const std::string& content) {
    std::string header = type + " " + std::to_string(content.size()) + '\0';
    std::string store_data = header + content;
    std::string sha1_hash = compute_sha1(store_data);

    std::string dir_name = sha1_hash.substr(0, 2);
    std::string file_name = sha1_hash.substr(2);
    
    fs::path object_dir = fs::path(".mygit/objects") / dir_name;
    fs::path object_path = object_dir / file_name;

    fs::create_directories(object_dir);
    
    std::ofstream out_file(object_path, std::ios::binary);
    if (out_file.is_open()) {
        out_file << store_data;
        out_file.close();
    } else {
        std::cerr << "Error: Could not write object to database.\n";
    }
    return sha1_hash;
}

void init_mygit() {
    try {
        fs::create_directories(".mygit/objects");
        fs::create_directories(".mygit/refs/heads");

        std::ofstream head_file(".mygit/HEAD");
        if (head_file.is_open()) {
            head_file << "ref: refs/heads/master\n";
            head_file.close();
        } else {
            std::cerr << "Error: Could not create HEAD file.\n";
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error creating directories: " << e.what() << '\n';
    }
}

// // Changed from void to std::string
// std::string hash_object(const std::string& filepath) {
//     std::ifstream file(filepath, std::ios::binary);
//     if (!file.is_open()) {
//         std::cerr << "Fatal: Cannot open file " << filepath << '\n';
//         return "";
//     }
//     std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
//     file.close();

//     std::string header = "blob " + std::to_string(content.size()) + '\0';
//     std::string store_data = header + content;
//     std::string sha1_hash = compute_sha1(store_data);

//     std::string dir_name = sha1_hash.substr(0, 2);
//     std::string file_name = sha1_hash.substr(2);
    
//     fs::path object_dir = fs::path(".mygit/objects") / dir_name;
//     fs::path object_path = object_dir / file_name;

//     fs::create_directories(object_dir);
    
//     std::ofstream out_file(object_path, std::ios::binary);
//     if (out_file.is_open()) {
//         out_file << store_data;
//         out_file.close();
//         return sha1_hash; // NEW: Return the hash instead of just printing it
//     } else {
//         std::cerr << "Error: Could not write object to database.\n";
//         return "";
//     }
// }

// UPDATED: Now incredibly clean thanks to our refactor!
std::string hash_object(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Fatal: Cannot open file " << filepath << '\n';
        return "";
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return write_object("blob", content);
}

// NEW: Reads a blob from the object database and prints its contents
void cat_file(const std::string& sha1_hash) {
    // Hashes must be exactly 40 characters long
    if (sha1_hash.length() != 40) {
        std::cerr << "Fatal: Not a valid SHA-1 hash\n";
        return;
    }

    // Split the hash to locate the file path
    std::string dir_name = sha1_hash.substr(0, 2);
    std::string file_name = sha1_hash.substr(2);
    fs::path object_path = fs::path(".mygit/objects") / dir_name / file_name;

    if (!fs::exists(object_path)) {
        std::cerr << "Fatal: Not a valid object name " << sha1_hash << '\n';
        return;
    }

    // Open the file in binary mode
    std::ifstream file(object_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Fatal: Cannot open object file\n";
        return;
    }

    // Read the entire file into memory
    std::string raw_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // The data format is "blob <size>\0<content>"
    // Find the null byte '\0' that separates the header from the actual text
    size_t null_byte_pos = raw_data.find('\0');
    if (null_byte_pos != std::string::npos) {
        // Print everything immediately after the null byte
        std::cout << raw_data.substr(null_byte_pos + 1);
    } else {
        std::cerr << "Fatal: Corrupted object file (no header found)\n";
    }
}

struct TreeEntry {
    std::string mode;
    std::string filename;
    std::string sha1;
};

// void write_tree() {
//     std::vector<TreeEntry> entries;

//     for (const auto& entry : fs::directory_iterator(".")) {
//         std::string filename = entry.path().filename().string();
        
//         // Ignore our database and the Makefile for now
//         if (filename == ".mygit" || filename == "Makefile") {
//             continue; 
//         }

//         if (entry.is_regular_file()) {
//             // Hash the file and store the blob in the database
//             std::string file_hash = hash_object(entry.path().string());
            
//             if (!file_hash.empty()) {
//                 // "100644" is Git's standard mode for a regular, non-executable file
//                 entries.push_back({"100644", filename, file_hash});
//                 std::cout << "Added to tree: 100644 " << filename << " -> " << file_hash << '\n';
//             }
//         }
//     }
// }

// CHANGED: Now returns std::string instead of void
std::string write_tree() {
    std::vector<TreeEntry> entries;

    for (const auto& entry : fs::directory_iterator(".")) {
        std::string filename = entry.path().filename().string();
        
        if (filename == ".mygit" || filename == "Makefile") {
            continue; 
        }

        if (entry.is_regular_file()) {
            std::string file_hash = hash_object(entry.path().string());
            if (!file_hash.empty()) {
                entries.push_back({"100644", filename, file_hash});
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](const TreeEntry& a, const TreeEntry& b) {
        return a.filename < b.filename;
    });

    std::string tree_content = "";
    for (const auto& entry : entries) {
        tree_content += entry.mode + " " + entry.filename + '\0' + hex_to_bytes(entry.sha1);
    }

    std::string tree_hash = write_object("tree", tree_content);
    return tree_hash; // NEW: Return the hash so the commit function can use it!
}

void commit(const std::string& message) {
    // 1. Generate the tree for the current directory state
    std::string tree_hash = write_tree();
    if (tree_hash.empty()) {
        std::cerr << "Fatal: Failed to write tree.\n";
        return;
    }

    // 2. Get the current Unix timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t current_time = std::chrono::system_clock::to_time_t(now);
    
    // 3. Format the commit content
    // Note: A real Git commit includes a "parent" hash, but we are skipping 
    // it for this MVP's very first commit to keep things simple.
    std::string commit_content = "tree " + tree_hash + '\n';
    commit_content += "author MyGit User <user@mygit.local> " + std::to_string(current_time) + " +0000\n";
    commit_content += "committer MyGit User <user@mygit.local> " + std::to_string(current_time) + " +0000\n\n";
    commit_content += message + '\n';

    // 4. Save the commit object using our universal function
    std::string commit_hash = write_object("commit", commit_content);

    // 5. Update the master branch to point to this new commit!
    std::ofstream ref_file(".mygit/refs/heads/master");
    if (ref_file.is_open()) {
        ref_file << commit_hash << '\n';
        ref_file.close();
        std::cout << "[master (root-commit) " << commit_hash.substr(0, 7) << "] " << message << '\n';
    } else {
        std::cerr << "Fatal: Could not update branch reference.\n";
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: ./mygit <command>\n";
        return 1;
    }

    std::string command = argv[1];

    if (command == "init") {
        init_mygit();
    } else if (command == "hash-object") {
        if (argc < 3) {
            std::cerr << "Usage: ./mygit hash-object <file>\n";
            return 1;
        }
        // NEW: Print the returned hash to the terminal
        std::string hash = hash_object(argv[2]);
        if (!hash.empty()) {
            std::cout << hash << '\n';
        }
    } else if (command == "cat-file") {
        // NEW: Route for the cat-file command
        if (argc < 3) {
            std::cerr << "Usage: ./mygit cat-file <hash>\n";
            return 1;
        }
        cat_file(argv[2]);
    } else if (command == "write-tree") {
        std::cout << write_tree() << '\n';
    } else if (command == "commit") {
        // NEW: Handle 'mygit commit -m "Message"'
        if (argc < 4 || std::string(argv[2]) != "-m") {
            std::cerr << "Usage: ./mygit commit -m \"<message>\"\n";
            return 1;
        }
        commit(argv[3]);
    }else {
        std::cerr << "Unknown command: " << command << '\n';
    }

    return 0;
}