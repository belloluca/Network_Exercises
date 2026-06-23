#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

#define PORT 8080
#define BUFFER 1024

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

void recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati\n");
        return;
    }
    buffer[n] = '\0';

    cout << "Risposta: " << buffer << endl;

}

int main(){

    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t len = sizeof(server_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("Errore nella socket\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if ((connect(sockfd, (struct sockaddr*)&server_addr, len)) < 0){
        perror("Errore nella connessione con il loadbalancer\n");
        return -1;
    }

    sendMsg(sockfd, "1+1");
    recvMsg(sockfd);

    close(sockfd);
    return 0;

}
