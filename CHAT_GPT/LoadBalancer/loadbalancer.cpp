#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>
#include <mutex>

using namespace std;

#define PORT 8080
#define BUFFER 1024

struct Server{

    string ip;
    int port;
    int count = 0;
    bool active = true;
    bool busy = false;

};

vector<Server> Servers = {

    {"127.0.0.1", 9001, 0, true, false},
    {"127.0.0.1", 9002, 0, true, false},
    {"127.0.0.1", 9003, 0, true, false}

};

int currentServer = 0;
mutex serverMutex;

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

string recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);

}

void printServerStatus() {
    lock_guard<mutex> lock(serverMutex);

    cout << "\n--- STATO SERVER ---" << endl;

    for (int i = 0; i < Servers.size(); i++) {
        cout << "Server " << Servers[i].port
             << " | count: " << Servers[i].count
             << " | active: " << Servers[i].active
             << " | busy: " << Servers[i].busy
             << endl;
    }

    cout << "--------------------\n" << endl;
}

int manageRequest(){

    lock_guard<mutex> lock(serverMutex);

    int bestServer = -1;
    int minCount = 1000;

    for (int i = 0; i < Servers.size(); i++) {
        if (Servers[i].active == true && Servers[i].busy == false) {
            if (Servers[i].count < minCount) {
                minCount = Servers[i].count;
                bestServer = i;
            }
        }
    }

    if (bestServer != -1){
        Servers[bestServer].count++;
        Servers[bestServer].busy = true;
    }

    return bestServer;

}

void handleClient(int sockfd){

    string response;
    Server server;

    string request = recvMsg(sockfd);

    if (request == "") {
        close(sockfd);
        return;
    }

    while (true){ 

        int index = manageRequest();

        if (index == -1) {
            sendMsg(sockfd, "Nessun server disponibile");
            cout << "Nessun server disponibile" << endl;
            close(sockfd);
            return;
        }

        server = Servers[index];

        int server_fd;
        struct sockaddr_in server_addr;

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0){
            perror("Errore nella socket\n");
            return;
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server.port);
        inet_pton(AF_INET, server.ip.c_str(), &server_addr.sin_addr);

        socklen_t len = sizeof(server_addr);
        if ((connect(server_fd, (struct sockaddr*)&server_addr, len)) < 0){
            perror("Errore nella connessione con il server\n");
            {

                lock_guard<mutex> lock(serverMutex);

                Servers[index].active = false;
                Servers[index].busy = false;

            }
            close(server_fd);
            continue;
        }

        sendMsg(server_fd, request);
        response = recvMsg(server_fd);

        {

            lock_guard<mutex> lock(serverMutex);

            Servers[index].busy = false;

        }

        printServerStatus();

        sendMsg(sockfd, response);

        close(server_fd);
        close(sockfd);
        return;

    }

}

int main(){

    int server_fd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        perror("Errore nella socket\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0){
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 10);
    cout << "Loadbalancer in ascolto...\n";

    while (true){

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }
        cout << "Gestendo il client\n";

        thread t(handleClient, new_sockfd);
        t.detach();

    }

    close(server_fd);
    return 0;

}
