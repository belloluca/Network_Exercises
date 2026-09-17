#include <iostream>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <thread>
#include <time.h>

using namespace std;

#define BUFFER 1024

string rcvMsg(int socket) {
    char buffer[BUFFER];
    
    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0) {
        perror("Errore nella ricezione dei dati\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);
}

void sendMsg(int socket, string msg) {
    if (send(socket, msg.c_str(), msg.size(), 0) < 0) {
        perror("Errore nell'invio dei dati\n");
        return;
    }
}

void send_stop(int socket, int id) {

    sleep(5);

    sendMsg(socket, "STOP ALARM: " + to_string(id));

}

void function (int socket) {
    ofstream file("alarms.txt", ios::app);

    while (true) {

        string data = rcvMsg(socket);

        file << data << endl;
        file.close();

        cout << "---ALLARME---" << endl << data << endl;

        int p1 = data.find(" ");
        int p2 = data.find(":");

        int id = stoi(data.substr(p1 + 1, p2 - p1 - 1));

        thread t(send_stop, socket, id);
        t.detach();

    }
    close(socket);
}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "usage: " << argv[0] << " <port>\n";
        return -1;
    }

    int port = stoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 1);
    cout << "COntrol node in ascolto\n";

    int new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
    if (new_sockfd < 0) {
        perror("Errore nella socket del central node\n");
        return -1;
    }

    function(new_sockfd);

    close(server_fd);
    return 0;

}