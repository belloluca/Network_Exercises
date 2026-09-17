#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>

using namespace std;

#define BUFFER 1024

void alarm(int socket) {

    char buffer[BUFFER];
    ofstream file("alarms.txt", ios::app);

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0) {
        perror("Errore nella ricezion dei dati\n");
        return;
    }
    buffer[n] = '\0';

    file << string(buffer) << endl;

    file.close();

    cout << "- ALLARME - " << buffer << endl;

}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "Usage " << argv[0] << " <local_port>\n";
        return -1;
    }

    int port = stoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in control_addr, central_addr;
    socklen_t len = sizeof(central_addr);

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(port);
    control_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&control_addr, sizeof(control_addr)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 1);
    cout << "Control Node in ascolto\n";

    int central_socket;

    while (true) {
        central_socket = accept(server_fd, (struct sockaddr*)&central_addr, &len);
        if (central_socket < 0) {
            perror("Errore nella connessione\n");
            continue;
        }
        cout << "Central Node collegato\n";

        alarm(central_socket);

        close(central_socket);
        cout << "Central Node disconnesso\n";
    }

    close(server_fd);
    return 0;

}