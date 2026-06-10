#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <vector>

using namespace std;

#define PORT 8080
#define BUFFER 1024

vector<sockaddr_in> Users;

bool sameClient(sockaddr_in a, sockaddr_in b){

    return a.sin_addr.s_addr == b.sin_addr.s_addr && a.sin_port == b.sin_port;

}

bool userExists(sockaddr_in client_addr) {
    for (auto user : Users) {
        if (sameClient(user, client_addr)) {
            return true;
        }
    }

    return false;
}

string recvMsg(int socket, sockaddr_in &client_addr){

    char buffer[BUFFER];
    socklen_t len = sizeof(client_addr);

    int n = recvfrom(socket, buffer, BUFFER - 1, 0, (struct sockaddr*)&client_addr, &len);
    if (n <= 0){
        perror("Errore nella ricezione dei dati\n");
        return "";
    }
    buffer[n] = '\0';

    if (!userExists(client_addr)){
        Users.push_back(client_addr);
    }  

    return string(buffer);

}

void sendMsg(int socket, sockaddr_in client_addr, string const msg){

    if ((sendto(socket, msg.c_str(), msg.size(), 0, (struct sockaddr*)&client_addr, sizeof(client_addr))) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

void broadcast(int socket, sockaddr_in client_addr, string const msg){

    for (auto user : Users){

        if (!sameClient(user, client_addr)){
            sendMsg(socket, user, msg);
        }

    }

}

int main(){

    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
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

    cout << "Server avviato...\n";

    string msg;

    while (true){

        msg = recvMsg(server_fd, client_addr);

        cout << "Messaggio del client: " << msg << endl;

        broadcast(server_fd, client_addr, msg);

    }

    close(server_fd);
    return 0;

}