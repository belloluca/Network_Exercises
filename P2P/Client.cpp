#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>

using namespace std;

#define PORT 8080
#define BUFFER 1024

void sendUDP(int socket, string peerIP, int peerPort){

    struct sockaddr_in peer;
    peer.sin_family = AF_INET;
    peer.sin_port = htons(peerPort);

    inet_pton(AF_INET, peerIP.c_str(), &peer.sin_addr);

    string msg;

    while (true){

        getline(cin, msg);

        if ((sendto(socket, msg.c_str(), msg.size(), 0, (struct sockaddr*)&peer, sizeof(peer))) < 0){
            perror("Errore nell'invio dei dati UDP\n");
            continue;
        }

        if (msg == "exit"){
            break;
        }

    }

}

void recvUDP(int socket){

    char buffer[BUFFER];

    while (true){

        struct sockaddr_in sender_addr;
        socklen_t len = sizeof(sender_addr);

        int n = recvfrom(socket, buffer, BUFFER - 1, 0, (struct sockaddr*)&sender_addr, &len);
        if (n <= 0){
            perror("Errore nella ricezione dei dati UDP\n");
            return;
        }
        buffer[n] = '\0';

        cout << "Messaggio ricevuto: " << buffer << endl;

    }

}

void sendMsg(int socket){

    string msg;

    getline(cin, msg);
    
    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    } 

}

void chat(string peerIP, int peerPort){

    cout << "Entrato nella chat" << endl;

    int udpsocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpsocket < 0){
        perror("Errore nella socket UDP\n");
        return;
    }

    int port;
    cout << "Inserisci la porta che hai comunicato al server: ";
    cin >> port;
    cin.ignore();

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if ((bind(udpsocket, (struct sockaddr*)&address, sizeof(address))) < 9){
        perror("Errore nel bind UDP\n");
        return;
    }

    thread t1(recvUDP, udpsocket);
    thread t2(sendUDP, udpsocket, peerIP, peerPort);

    t2.join();
    t1.detach();

    close(udpsocket);

}

void recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati\n");
        return;
    }
    buffer[n] = '\0';

    cout << buffer << endl;

    if (string(buffer).find("Peer info") != string::npos){

        int pos1 = string(buffer).find(":");
        int pos2 = string(buffer).find(",");
        string peerIP = string(buffer).substr(pos1 + 2, pos2 - (pos1 + 2));
        pos1 = string(buffer).find(":", pos2);
        int peerPort = stoi(string(buffer).substr(pos1 + 2));

        cout << endl << endl << "ip peer: " << peerIP << " porta: " << peerPort << endl;

        chat(peerIP, peerPort);
        
    }

}

void function(int socket){

    recvMsg(socket);
    sendMsg(socket);

    recvMsg(socket);
    sendMsg(socket);

    recvMsg(socket);
    sendMsg(socket);

    recvMsg(socket);

    close(socket);

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
        perror("Errore nella connessione con il server\n");
        return -1;
    }
    cout << "Client connesso al server...\n";

    thread t(function, sockfd);
    t.join();

    close(sockfd);
    return 0;


}
