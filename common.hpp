#ifndef COMMON_HPP
#define COMMON_HPP

#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/socket.h>

using namespace std;

// Constants
constexpr int MAX_USER_LEN = 8;
constexpr int MAX_SUBJECT_LEN = 80;

// Protocol commands
const string CMD_SEND = "SEND";
const string CMD_LIST = "LIST";
const string CMD_READ = "READ";
const string CMD_DEL = "DEL";
const string CMD_QUIT = "QUIT";

// Protocol responses
const string RESP_OK = "OK";
const string RESP_ERR = "ERR";

// Utility functions
inline void die(const string& msg) {
    cerr << "ERROR: " << msg << endl;
    exit(1);
}

inline bool send_line(int fd, const string& line) {
    string data = line + "\n";
    return send(fd, data.c_str(), data.size(), 0) > 0;
}

inline string read_line(int fd) {
    string buffer;
    char ch;
    while (recv(fd, &ch, 1, 0) > 0) {
        if (ch == '\n') break;
        if (ch != '\r') buffer += ch;
    }
    return buffer;
}

inline bool valid_user(const string& u) {
    if (u.empty() || u.size() > MAX_USER_LEN) return false;
    for (char c : u)
        if (!isdigit(c) && !islower(c)) return false;
    return true;
}

#endif // COMMON_HPP
