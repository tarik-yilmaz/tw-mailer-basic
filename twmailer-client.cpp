// twmailer-client.cpp
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "common.hpp"

using namespace std;




// State to remember last listed user
string last_list_user = "";




int connect_to_server(const string& ip, const string& port) {
    addrinfo hints = {}, *res;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    
    if (getaddrinfo(ip.c_str(), port.c_str(), &hints, &res)) die("getaddrinfo failed");
    
    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (connect(fd, res->ai_addr, res->ai_addrlen)) die("connect failed");
    
    freeaddrinfo(res);
    return fd;
}

void cmd_send(int fd) {
    string sender, receiver, subject, line;
    
    cout << "Sender: "; getline(cin, sender);
    cout << "Receiver: "; getline(cin, receiver);
    cout << "Subject: "; getline(cin, subject);
    
    if (subject.size() > MAX_SUBJECT_LEN) die("Subject too long");
    if (!valid_user(sender) || !valid_user(receiver)) die("Invalid username");
    
    send_line(fd, CMD_SEND);
    send_line(fd, sender);
    send_line(fd, receiver);
    send_line(fd, subject);
    
    cout << "Message (end with '.'):" << endl;
    while (getline(cin, line) && line != ".") send_line(fd, line);
    send_line(fd, ".");
    
    cout << read_line(fd) << endl;
}

void perform_list(int fd, const string& user) {
    send_line(fd, CMD_LIST);
    send_line(fd, user);
    
    int count = stoi(read_line(fd));
    cout << "Messages: " << count << endl;
    for (int i = 1; i <= count; i++)
        cout << i << ") " << read_line(fd) << endl;
}

void cmd_list(int fd) {
    string user;
    cout << "Username: "; getline(cin, user);
    if (!valid_user(user)) die("Invalid username");
    
    perform_list(fd, user);
    last_list_user = user;
}

void cmd_read(int fd) {
    string user, num;
    cout << "Username: "; getline(cin, user);
    
    if (!valid_user(user)) die("Invalid username");
    
    // Auto-List if we haven't listed this user recently
    if (user != last_list_user) {
        cout << "[Auto-List for " << user << "]" << endl;
        perform_list(fd, user);
        last_list_user = user;
    }
    
    cout << "Message#: "; getline(cin, num);
    
    send_line(fd, CMD_READ);
    send_line(fd, user);
    send_line(fd, num);
    
    if (read_line(fd) == RESP_OK) {
        string line;
        while ((line = read_line(fd)) != ".") cout << line << endl;
        cout << "." << endl;
    } else {
        cout << "ERR" << endl;
    }
}

void cmd_del(int fd) {
    string user, num;
    cout << "Username: "; getline(cin, user);
    
    if (!valid_user(user)) die("Invalid username");
    
    // Auto-List if we haven't listed this user recently
    if (user != last_list_user) {
        cout << "[Auto-List for " << user << "]" << endl;
        perform_list(fd, user);
        last_list_user = user;
    }
    
    cout << "Message#: "; getline(cin, num);
    
    send_line(fd, CMD_DEL);
    send_line(fd, user);
    send_line(fd, num);
    
    cout << read_line(fd) << endl;
}

int main(int argc, char** argv) {
    if (argc != 3) die("Usage: ./twmailer-client <ip> <port>");
    
    int fd = connect_to_server(argv[1], argv[2]);
    cout << "Connected. Commands: SEND, LIST, READ, DEL, QUIT" << endl;
    
    while (true) {
        cout << "> "; string cmd; getline(cin, cmd);
        
        for (char &c : cmd) c = toupper(c);
        
        if (cmd == CMD_SEND) cmd_send(fd);
        else if (cmd == CMD_LIST) cmd_list(fd);
        else if (cmd == CMD_READ) cmd_read(fd);
        else if (cmd == CMD_DEL) cmd_del(fd);
        else if (cmd == CMD_QUIT) { send_line(fd, CMD_QUIT); break; }
        else if (!cmd.empty()) cout << "Unknown command" << endl;
    }
    
    close(fd);
    cout << "Disconnected." << endl;
    return 0;
}