#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <set>
#include <mutex>
#include <thread>

using namespace std;

#define PORT 8080
#define BUFFER 1024

set<int> Users;
mutex serverMutex;

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

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

void broadcast(int socket, string const msg){

    lock_guard<mutex> lock(serverMutex);

    for (auto user : Users){

        if (user != socket){
            sendMsg(user, msg);
        }

    }

}

void handleClient(int socket){

    string msg;

    while (true){

        msg = recvMsg(socket);

        cout << "Messaggio del client: " << msg << endl;

        broadcast(socket, msg);

    }

    close(socket);

}

int main(){

    int server_fd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        perror("Erorre nella socket\n");
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
    cout << "Server in ascolto...\n";

    while (true){

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }
        cout << "Client connesso...\n";

        {

            lock_guard<mutex> lock(serverMutex);

            Users.insert(new_sockfd);
            
        }

        thread t(handleClient, new_sockfd);
        t.detach();

    }

    close(server_fd);
    return 0;

}
