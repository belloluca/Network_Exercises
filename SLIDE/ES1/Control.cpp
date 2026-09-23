#include <iostream>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include <mutex>
#include "protocol.h"

using namespace std;

mutex serverMutex;

struct sockaddr_in server_addr, client_addr;
int server_fd, new_sockfd;

Message rcvMsg() {
    Message msg;

    int n = recv(new_sockfd, &msg, sizeof(msg), 0);
    if (n != sizeof(msg)) {
        perror("Errore nella ricezione dei messaggi");
    }

    return msg;
}

void function() {
    ofstream file("alarm.txt", ios::app);
    Message msg;

    while (true) {
        msg = rcvMsg();

        cout << "|ALLARME| Sensore " << msg.id << ": " << msg.temp << " | " << msg.hum << " | " << msg.air << endl;

        {
            lock_guard<mutex> lock(serverMutex);

            file << "Sensore " << msg.id << ": " << msg.temp << " | " << msg.hum << " | " << msg.air << endl;

            file.close();
        }
    }

}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "usage: " << argv[0] << " <local port>\n";
        return -1;
    }

    int port = stoi(argv[1]);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 1);
    cout << "Control Node in ascolto\n";

    socklen_t len = sizeof(client_addr);
    new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
    if (new_sockfd < 0) {
        perror("Errore nella connessione sul Central Node\n");
        return -1;
    }
    cout << "Central Node connesso\n";

    function();

    close(server_fd);
    close(new_sockfd);
    return 0;

}