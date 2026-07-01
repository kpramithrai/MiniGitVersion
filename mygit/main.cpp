#include <iostream>
#include <filesystem>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>

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

void hash_object(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Fatal: Cannot open file " << filepath << '\n';
        return;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    std::string header = "blob " + std::to_string(content.size()) + '\0';
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
        std::cout << sha1_hash << '\n'; 
    } else {
        std::cerr << "Error: Could not write object to database.\n";
    }
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
        hash_object(argv[2]);
    } else if (command == "cat-file") {
        // NEW: Route for the cat-file command
        if (argc < 3) {
            std::cerr << "Usage: ./mygit cat-file <hash>\n";
            return 1;
        }
        cat_file(argv[2]);
    } else {
        std::cerr << "Unknown command: " << command << '\n';
    }

    return 0;
}