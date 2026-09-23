#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include "protocol.h"

using namespace std;

int id;

int sockfd;
struct sockaddr_in central_addr;

void function() {

    int temp, hum;
    string air;

    Message msg;
    msg.id = id;

    while (true) {
        memset(&msg, '\0', sizeof(msg));

        temp = rand() % 45;
        hum = rand() % 100;
        air = (rand() % 2) ? "GOOD" : "POOR";

        msg.temp = temp;
        msg.hum = hum;
        strncpy(msg.air, air.c_str(), sizeof(msg.air));

        cout << "Sensore " << id << ": " << temp << " | " << hum << " | " << air << endl;

        if (sendto(sockfd, &msg, sizeof(msg), 0, (struct sockaddr*)&central_addr, sizeof(central_addr)) < 0) {
            perror("Errore nell'invio dei dati\n");
        }

        sleep(3);
    }

}

int main(int argc, char* argv[]) {

    srand(time(0));

    if (argc != 3) {
        cout << "usage: " << argv[0] << " <id> <central port>\n";
        return -1; 
    }

    id = stoi(argv[1]);
    int port = stoi(argv[2]);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    central_addr.sin_family = AF_INET;
    central_addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.0", &central_addr.sin_addr);

    function();

    close(sockfd);
    return 0;

}