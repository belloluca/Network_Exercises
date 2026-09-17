#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <iostream>

using namespace std;

#define BUFFER 1024

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "usage: " << argv[0] << " <local port>\n";
        return -1;
    }

    int port = stoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in server_address, client_address;
    socklen_t len = sizeof(client_address);
    
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_address, len) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }
    cout << "Server in ascolto\n";

    char buffer[BUFFER];
    while (true) {
        int n = recvfrom(server_fd, buffer, BUFFER - 1, 0, (struct sockaddr*)&client_address, &len);
        if (n < 0) {
            perror("Errore nella ricezione dei dati\n");
            continue;
        }
        buffer[n] = '\0';

        cout << buffer << endl;
    }

    close(server_fd);
    return 0;

}