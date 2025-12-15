// twmailer-client.cpp
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>
#include <string>

using namespace std;

void die(const string& msg) {
    cerr << "ERROR: " << msg << endl;
    exit(1);
}

bool send_line(int fd, const string& line) {
    string data = line + "\n";
    return send(fd, data.c_str(), data.size(), 0) > 0;
}

string read_line(int fd) {
    string buffer;
    char ch;
    while (recv(fd, &ch, 1, 0) > 0) {
        if (ch == '\n') break;
        if (ch != '\r') buffer += ch;
    }
    return buffer;
}

bool valid_user(const string& u) {
    if (u.empty() || u.size() > 8) return false;
    for (char c : u)
        if (!isdigit(c) && !islower(c)) return false;
    return true;
}

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
    
    if (subject.size() > 80) die("Subject too long");
    if (!valid_user(sender) || !valid_user(receiver)) die("Invalid username");
    
    send_line(fd, "SEND");
    send_line(fd, sender);
    send_line(fd, receiver);
    send_line(fd, subject);
    
    cout << "Message (end with '.'):" << endl;
    while (getline(cin, line) && line != ".") send_line(fd, line);
    send_line(fd, ".");
    
    cout << read_line(fd) << endl;
}

void cmd_list(int fd) {
    string user;
    cout << "Username: "; getline(cin, user);
    if (!valid_user(user)) die("Invalid username");
    
    send_line(fd, "LIST");
    send_line(fd, user);
    
    int count = stoi(read_line(fd));
    cout << "Messages: " << count << endl;
    for (int i = 1; i <= count; i++)
        cout << i << ") " << read_line(fd) << endl;
}

void cmd_read(int fd) {
    string user, num;
    cout << "Username: "; getline(cin, user);
    cout << "Message#: "; getline(cin, num);
    
    if (!valid_user(user)) die("Invalid username");
    
    send_line(fd, "READ");
    send_line(fd, user);
    send_line(fd, num);
    
    if (read_line(fd) == "OK") {
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
    cout << "Message#: "; getline(cin, num);
    
    if (!valid_user(user)) die("Invalid username");
    
    send_line(fd, "DEL");
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
        
        if (cmd == "SEND") cmd_send(fd);
        else if (cmd == "LIST") cmd_list(fd);
        else if (cmd == "READ") cmd_read(fd);
        else if (cmd == "DEL") cmd_del(fd);
        else if (cmd == "QUIT") { send_line(fd, "QUIT"); break; }
        else if (!cmd.empty()) cout << "Unknown command" << endl;
    }
    
    close(fd);
    cout << "Disconnected." << endl;
    return 0;
}