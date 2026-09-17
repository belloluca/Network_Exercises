#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <thread>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

using namespace std;

#define BUFFER 1024

int id;

string next_ip;
int next_port;

string server_ip;
int server_port;
struct sockaddr_in next_address;

void sendMsg(int socket, string msg) {
    next_address.sin_family = AF_INET;
    next_address.sin_port = htons(next_port);
    inet_pton(AF_INET, next_ip.c_str(), &next_address.sin_addr);

    if (sendto(socket, msg.c_str(), msg.size(), 0, (struct sockaddr*)&next_address, sizeof(next_address)) < 0) {
        perror("Errore nell'invio dei dati\n");
        return;
    }
}

void rcvMsg(int socket) {
    struct sockaddr_in next_address;
    inet_pton(AF_INET, next_ip.c_str(), &next_address);
    socklen_t len = sizeof(next_address);

    char buffer[BUFFER];
    while (true) {
        int n = recvfrom(socket, buffer, BUFFER - 1, 0, (struct sockaddr*)&next_address, &len);
        if (n < 0) {
            perror("Errore nella ricezione dei dati\n");
            continue;
        }
        buffer[n] = '\0';

        cout << buffer << endl;
    }
}

int main(int argc, char* argv[]) {

    if (argc != 7) {
        cout << "usage: " << argv[0] << " <id> <local port> <next ip> <next port> <server ip> <server port>\n";
        return -1;
    }

    id = stoi(argv[1]);

    int local_port = stoi(argv[2]);

    next_ip = argv[3];
    next_port = stoi(argv[4]);

    server_ip = argv[5];
    server_port = stoi(argv[6]);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in node_address;
    socklen_t len = sizeof(node_address);

    node_address.sin_family = AF_INET;
    node_address.sin_port = htons(local_port);
    inet_pton(AF_INET, "127.0.0.1", &node_address.sin_addr);

    if (bind(sockfd, (struct sockaddr*)&node_address, len) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }

    cout << "---Nodo attivato---\n";



}