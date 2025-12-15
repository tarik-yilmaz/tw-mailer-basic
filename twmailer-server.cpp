#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "common.hpp"

using namespace std;
namespace fs = std::filesystem;

// --- Helper Functions ---

// Generate timestamp string: YYYYMMDD_HHMMSS
string get_timestamp() {
    auto now = time(nullptr);
    auto tm = *localtime(&now);
    ostringstream oss;
    oss << put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

// Generate filename: YYYYMMDD_HHMMSS_SEQ.txt (Optimistic Write)
string generate_filename(const string& user_dir) {
    string timestamp = get_timestamp();
    int seq = 1;
    while (true) {
        ostringstream oss;
        oss << timestamp << "_" << setfill('0') << setw(3) << seq << ".txt";
        string filename = oss.str();
        fs::path filepath = fs::path(user_dir) / filename;
        if (!fs::exists(filepath)) {
            return filepath.string();
        }
        seq++;
    }
}

// Get sorted list of message files for a user
vector<fs::path> get_user_messages(const string& user_dir) {
    vector<fs::path> files;
    if (!fs::exists(user_dir)) return files;
    
    for (const auto& entry : fs::directory_iterator(user_dir)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            files.push_back(entry.path());
        }
    }
    sort(files.begin(), files.end()); // Guarantee order
    return files;
}

// --- Command Handlers ---

void handle_send(int fd, const string& spool_dir) {
    string sender = read_line(fd);
    string receiver = read_line(fd);
    string subject = read_line(fd);
    
    // Read Body
    string body, line;
    while ((line = read_line(fd)) != ".") {
        body += line + "\n";
    }

    // Validation
    if (sender.empty() || receiver.empty() || subject.empty()) {
        send_line(fd, RESP_ERR);
        return;
    }

    // Ensure spool/receiver directory exists
    string user_dir = fs::path(spool_dir) / receiver;
    try {
        fs::create_directories(user_dir);
    } catch (...) {
        send_line(fd, RESP_ERR);
        return;
    }

    // Write to file
    try {
        string filepath = generate_filename(user_dir);
        ofstream file(filepath);
        if (!file.is_open()) {
            send_line(fd, RESP_ERR);
            return;
        }
        file << subject << endl; // Line 1: Subject
        file << body;            // Rest: Body
        file.close();
        send_line(fd, RESP_OK);
    } catch (...) {
        send_line(fd, RESP_ERR);
    }
}

void handle_list(int fd, const string& spool_dir) {
    string username = read_line(fd);
    string user_dir = fs::path(spool_dir) / username;

    vector<fs::path> files = get_user_messages(user_dir);
    send_line(fd, to_string(files.size())); // Send Count

    for (const auto& filepath : files) {
        ifstream file(filepath);
        string subject;
        if (getline(file, subject)) {
            send_line(fd, subject);
        } else {
            send_line(fd, "(No Subject)"); // Should not happen with well-formed files
        }
    }
}

void handle_read(int fd, const string& spool_dir) {
    string username = read_line(fd);
    string msg_num_str = read_line(fd);
    int msg_num;

    try {
        msg_num = stoi(msg_num_str);
    } catch (...) {
        send_line(fd, RESP_ERR);
        return;
    }

    string user_dir = fs::path(spool_dir) / username;
    vector<fs::path> files = get_user_messages(user_dir);

    if (msg_num < 1 || msg_num > (int)files.size()) {
        send_line(fd, RESP_ERR);
        return;
    }

    // Index is msg_num - 1
    fs::path filepath = files[msg_num - 1];
    ifstream file(filepath);
    if (!file.is_open()) {
        send_line(fd, RESP_ERR);
        return;
    }

    send_line(fd, RESP_OK);
    string line;
    while (getline(file, line)) {
        send_line(fd, line);
    }
    send_line(fd, ".");
}

void handle_del(int fd, const string& spool_dir) {
    string username = read_line(fd);
    string msg_num_str = read_line(fd);
    int msg_num;

    try {
        msg_num = stoi(msg_num_str);
    } catch (...) {
        send_line(fd, RESP_ERR);
        return;
    }

    string user_dir = fs::path(spool_dir) / username;
    vector<fs::path> files = get_user_messages(user_dir);

    if (msg_num < 1 || msg_num > (int)files.size()) {
        send_line(fd, RESP_ERR);
        return;
    }

    // Remove file
    try {
        fs::remove(files[msg_num - 1]);
        send_line(fd, RESP_OK);
    } catch (...) {
        send_line(fd, RESP_ERR);
    }
}

// --- Main Server Logic ---

void handle_client(int client_fd, const string& spool_dir) {
    while (true) {
        string command = read_line(client_fd);
        if (command.empty()) break; // Disconnect
        
        if (command == CMD_SEND) {
            handle_send(client_fd, spool_dir);
        } else if (command == CMD_LIST) {
            handle_list(client_fd, spool_dir);
        } else if (command == CMD_READ) {
            handle_read(client_fd, spool_dir);
        } else if (command == CMD_DEL) {
            handle_del(client_fd, spool_dir);
        } else if (command == CMD_QUIT) {
            break;
        } else {
             // Unknown command
            send_line(client_fd, RESP_ERR); 
        }
    }
    close(client_fd);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cerr << "Usage: ./twmailer-server <port> <mail-spool-directory>" << endl;
        return 1;
    }

    int port = stoi(argv[1]);
    string spool_dir = argv[2];

    try {
        fs::create_directories(spool_dir);
    } catch (const exception& e) {
        die("Could not create spool directory: " + string(e.what()));
    }

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) die("socket failed");

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (sockaddr*)&addr, sizeof(addr)) == -1) die("bind failed");
    if (listen(server_fd, 5) == -1) die("listen failed");

    cout << "Server started on port " << port << " with spool directory " << spool_dir << endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t len = sizeof(client_addr);
        int client_fd = accept(server_fd, (sockaddr*)&client_addr, &len);
        if (client_fd == -1) {
            cerr << "accept failed" << endl;
            continue;
        }
        
        cout << "Client connected: " << inet_ntoa(client_addr.sin_addr) << endl;
        handle_client(client_fd, spool_dir);
        cout << "Client disconnected" << endl;
    }

    close(server_fd);
    return 0;
}
