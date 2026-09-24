#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "protocol.h"

using namespace std;

#define PORT 8080

struct sockaddr_in address, next_address, server_address;
int sockfd;
int id;

string next_ip;
int next_port;

void InoltroServer(Message msg, sockaddr_in server_address) {
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr);

    sendMsg(sockfd, server_address, msg);
}

void Inoltro(Message msg, sockaddr_in next_address) {
    next_address.sin_family = AF_INET;
    next_address.sin_port = htons(next_port);
    inet_pton(AF_INET, next_ip.c_str(), &next_address.sin_addr);
        
    sendMsg(sockfd, next_address, msg);

}

void Chat() {
    next_address.sin_family = AF_INET;
    next_address.sin_port = htons(next_port);
    inet_pton(AF_INET, next_ip.c_str(), &next_address.sin_addr);

    Message msg;

    while (true) {
        memset(&msg, '\0', sizeof(msg));
        msg.id = id;

        string data;
        getline(cin, data);
        
        int p1 = data.find(" ");
        int destinatario = stoi(data.substr(0, p1));
        msg.dest = destinatario;

        string message = data.substr(p1 + 1);
        strncpy(msg.content, message.c_str(), sizeof(msg.content));
        
        sendMsg(sockfd, next_address, msg);
        InoltroServer(msg, server_address);
    }

}

void viewMessage() {
    Message msg;

    while (true) {
        memset(&msg, '\0', sizeof(msg));

        msg = rcvMsg(sockfd, address);

        if (msg.id == id) {
            continue;
        }
        else if (msg.dest == id) {
            cout << "Nodo " << msg.id << ": " << msg.content << endl;
        } 
        else if (msg.dest == 0 && msg.id != id) {
            cout << "Nodo " << msg.id << ": " << msg.content << endl;
            Inoltro(msg, next_address);
        }
        else if (msg.dest != id) {
            Inoltro(msg, next_address);
        }
    }

}

int main(int argc, char* argv[]) {

    if (argc != 5) {
        cout << "usage: " << argv[0] << " <id> <local port> <next ip> <next port>\n";
        return -1;
    }

    cout << "L'id '0' equivale ad una comunicazione broadcast" << endl << "Altrimenti puoi indicare l'id del nodo" << endl;

    id = stoi(argv[1]);
    int local_port = stoi(argv[2]);

    next_ip = argv[3];
    next_port = stoi(argv[4]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    
    socklen_t len = sizeof(next_address);

    address.sin_family = AF_INET;
    address.sin_port = htons(local_port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sockfd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }
    cout << "Nodo attivo\n";

    thread t1(viewMessage);
    thread t2(Chat);

    t1.join(); 
    t2.join();

    close(sockfd);
    return 0;

}