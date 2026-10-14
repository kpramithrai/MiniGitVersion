# TrackIt — Mini Git Version Control System

TrackIt is a lightweight, Git-inspired version control system built from scratch using C++17. I built this project to understand what happens behind the scenes when we commit files, store snapshots, and restore earlier versions of our code.

Instead of using Git's existing implementation, TrackIt recreates some of its core concepts, including SHA-1 hashing, object storage, tree snapshots, and commit history.

## 🚀 Features

- **Repository Initialization:** Create a local repository using `./mygit init`.
- **Blob Storage:** Store file contents as objects identified by their SHA-1 hashes.
- **Tree Objects:** Represent the files in a project snapshot.
- **Commit Creation:** Save snapshots with commit messages and parent commit references.
- **Commit History:** View previous commits using `./mygit log`.
- **File Restoration:** Restore an individual file from a previous commit.
- **Persistent Storage:** Store repository objects and references on the filesystem.
- **Command-Line Interface:** Perform version control operations directly from the terminal.

## 🛠️ Tech Stack

- **Language:** C++17
- **Hashing:** SHA-1
- **Cryptographic Library:** OpenSSL
- **Build Tool:** GNU Make
- **Core Concepts:** File systems, object storage, hashing, snapshots, and commit history

## ⚙️ Getting Started

### Prerequisites

Make sure you have a C++17-compatible compiler, GNU Make, and the OpenSSL development libraries installed.

On Ubuntu or Debian, run:

```bash
sudo apt update
sudo apt install g++ make libssl-dev
```

### Build the Project

Clone the repository and enter the project directory:

```bash
git clone https://github.com/kpramithrai/MiniGitVersion.git
cd MiniGitVersion
```

Compile the program:

```bash
make
```

This builds the executable named `mygit`.

## 💻 Available Commands

### 1. Initialize a Repository

```bash
./mygit init
```

Creates the `.mygit` directory and initializes the repository's object database and references.

### 2. Store a File as a Blob

```bash
./mygit hash-object test.txt
```

Reads the file, stores its contents as a blob object, and prints its SHA-1 hash.

### 3. Read an Object

```bash
./mygit cat-file <object-hash>
```

Displays the contents of the specified object using its full SHA-1 hash.

### 4. Create a Tree Snapshot

```bash
./mygit write-tree
```

Creates a tree object representing the regular files in the current directory and prints its hash.

### 5. Create a Commit

```bash
./mygit commit -m "Initial commit"
```

Creates a snapshot, stores a commit object, links it to the previous commit when available, and updates the current branch reference.

### 6. View Commit History

```bash
./mygit log
```

Displays the commit history by following parent commit references.

### 7. Restore a File from a Previous Commit

```bash
./mygit checkout <commit-hash> test.txt
```

Restores `test.txt` from the specified commit. Replace `<commit-hash>` with the full hash of the commit you want to restore from.

**Note:** Checkout overwrites the specified file in the working directory.

## 🧠 How It Works

TrackIt uses a simplified version of Git's object model:

- **Blob:** Stores the contents of a file.
- **Tree:** Stores references to file objects and represents a snapshot.
- **Commit:** Records a snapshot, commit metadata, and the parent commit.
- **HEAD:** Identifies the current branch reference.
- **Branch Reference:** Points to the latest commit in the branch.

The relationship between these objects allows TrackIt to save snapshots and retrieve file contents from earlier commits.

## 🔄 Example Workflow

```bash
# Initialize the repository
./mygit init

# Create a file
echo "Hello, TrackIt!" > test.txt

# Store the file as a blob
./mygit hash-object test.txt

# Create the first commit
./mygit commit -m "Initial commit"

# Modify the file
echo "Updated content!" > test.txt

# Create another commit
./mygit commit -m "Update test file"

# View commit history
./mygit log

# Restore the file from an earlier commit
./mygit checkout <first-commit-hash> test.txt
```

Run these commands from the initialized project directory. Record the first commit's hash before creating the second commit so you can restore the earlier version.

## 📂 Repository Structure

```text
MiniGitVersion/
├── .mygit/          # Repository metadata and objects
│   ├── objects/     # Stored blob, tree, and commit objects
│   ├── refs/        # Branch references
│   └── HEAD         # Current branch reference
├── main.cpp         # Core C++ implementation
├── Makefile         # Build configuration
├── README.md        # Project documentation
└── mygit            # Compiled executable
```

The `.mygit` directory and `mygit` executable are generated locally.

## 📚 What I Learned

Building TrackIt helped me understand how version control systems manage data internally. Some of the key concepts I explored were:

- Using SHA-1 hashing for content-addressable storage.
- Representing files and snapshots through blobs and trees.
- Linking commits to maintain a navigable history.
- Using references to track the current version.
- Restoring historical file contents by traversing stored objects.
- Handling filesystem operations and binary object data in C++.

The most interesting part was seeing how these individual components work together to make version history possible.

## 🔮 Future Improvements

- Implement a staging area for selective commits.
- Add branch creation and switching.
- Implement file-diff functionality.
- Support recursive directory trees.
- Restore entire project snapshots during checkout.
- Add merge support and more comprehensive automated tests.

## 👨‍💻 Author

**Pramith Rai**

- GitHub: [@kpramithrai](https://github.com/kpramithrai)

---

*TrackIt started with a simple question: what actually happens when we commit our code in Git? This project was my way of exploring that question by building a simplified version control system from the ground up.*

