#define MAX_MSG 256
#include <iostream>
#include <netinet/in.h>

#pragma pack(push, 1)
struct Message {
    int id; // 4 byte
    int dest; // 4 byte
    char content[MAX_MSG]; // 256 byte
}; // 264 byte
#pragma pack(pop)

Message rcvMsg(int socket, sockaddr_in address) {
    socklen_t len = sizeof(address);
    Message msg;

    int n = recvfrom(socket, &msg, sizeof(msg), 0, (struct sockaddr*)&address, &len);
    if (n != sizeof(msg)) {
        perror("Errore nella ricezione dei dati\n");
    }
    return msg;
}

void sendMsg(int socket, sockaddr_in address, Message msg) {
    socklen_t len = sizeof(address);
    if (sendto(socket, &msg, sizeof(msg), 0, (struct sockaddr*)&address, len) < 0) {
        perror("Errore nell'invio dei dati\n");
    }
}