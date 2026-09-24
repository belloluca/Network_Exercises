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

int main(int argc, char* argv[]) {

    if (argc != 1) {
        cout << "usage: " << argv[0] << endl;
        return -1;
    }

    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in server_address, client_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }
    cout << "Server in ascolto\n";

    Message msg;
    while (true) {
        memset(&msg, '\0', sizeof(msg));
        msg = rcvMsg(server_fd, client_address);

        cout << "Nodo " << msg.id << " a " << msg.dest << ": " << msg.content << endl;

    }

    close(server_fd);
    return 0;

}