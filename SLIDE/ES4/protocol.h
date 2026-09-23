#include <arpa/inet.h>
#include <stdio.h>

#define MAX_NAME 24
#define MAX_MSG 256
#define MAX_PASSWORD 24
#define COMAND 9

#pragma pack(push, 1)
struct Message {
    char comand[COMAND]; // 9 byte
    char name[MAX_NAME]; // 24 byte
    char password[MAX_PASSWORD]; // 24 byte
    char message[MAX_MSG]; // 256 byte
}; // 313 byte
#pragma pack(pop)

void sendMsg(int socket, Message msg) {
    if (send(socket, &msg, sizeof(msg), 0) < 0) {
        perror("Errore nell'invio dei dati\n");
    }
}

Message rcvMsg(int socket) {
    Message msg;
    int n = recv(socket, &msg, sizeof(msg), 0);
    if (n != sizeof(msg)) {
        perror("Errore nella ricezione dei messaggi\n");
    }

    return msg;
}