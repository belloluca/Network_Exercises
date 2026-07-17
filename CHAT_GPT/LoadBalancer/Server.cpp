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

#define BUFFER 1024

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

    cout << "Richiesta: " << buffer << endl;

    return string(buffer);

}

void handleClient(int socket){

    string request;

    request = recvMsg(socket);

    int n1, n2;

    n1 = stoi(request.substr(1));
    n2 = stoi(request.substr(1, 2));

    int response = n1 + n2;

    sendMsg(socket, to_string(response));

    close(socket);

}

int main(int argc, char *argv[]){

    int server_fd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);
    int count = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        perror("Errore nella socket\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(stoi(argv[1]));
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
        cout << "Gestendo il client\n";
        count++;
        cout << "Richiesta n: " << count << endl;

        thread t(handleClient, new_sockfd);
        t.detach();

    }

    close(server_fd);
    return 0;


}
