#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <map>

using namespace std;

#define PORT 8080
#define BUFFER 1024

struct User{

    string email;
    string ip;
    int port;
    int socket = -1;

};

map<string, User> Users;
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

void exchange(int socket, string const sender){

    sendMsg(socket, "Con chi vuoi parlare? ");
    string peer = recvMsg(socket);
    
    string senderIP, peerIP;
    int senderPort, peerPort;
    int senderSocket, peerSocket;

    {

        lock_guard<mutex> lock(serverMutex);

        senderIP = Users[sender].ip;
        senderPort = Users[sender].port;
        senderSocket = Users[sender].socket;

        peerIP = Users[peer].ip;
        peerPort = Users[peer].port;
        peerSocket = Users[peer].socket;

    }

    sendMsg(senderSocket, "Peer info - IP: " + peerIP + ", Porta: " + to_string(peerPort));
    sendMsg(peerSocket, "Peer info - IP: " + senderIP + ", Porta: " + to_string(senderPort));

    {

        lock_guard<mutex> lock(serverMutex);
        
        Users[sender].socket = -1;
        Users[peer].socket = -1;

    }

    return;

}

int main(){

    int server_fd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(server_addr);
    User user;

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
    cout << "Server in ascolto...\n";

    while (true){

        string email;
        int port;

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }
        cout << "Client connesso...\n";

        sendMsg(new_sockfd, "Inserisci email: ");
        email = recvMsg(new_sockfd);

        sendMsg(new_sockfd, "Inserisci la porta P2P: ");
        port = stoi(recvMsg(new_sockfd));

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);

        {

            lock_guard<mutex> lock(serverMutex);
            
            user.email = email;
            user.port = port;
            user.ip = string(ip);
            user.socket = new_sockfd;

            Users[email] = user;

        }

        thread t(exchange, new_sockfd, email);
        t.detach();

    }

    close(server_fd);
    return 0;

}