#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

int id;

#pragma pack(push, 1)
struct Message {
    int id; // 4 byte
    int temp; // 4 byte
    int hum; // 4 byte
    char air[5]; // 5 byte
}; // 17 byte totali
#pragma pack(pop)

void function(int socket, int central_port) {

    sockaddr_in central_addr;
    central_addr.sin_family = AF_INET;
    central_addr.sin_port = htons(central_port);
    inet_pton(AF_INET, "127.0.0.1", &central_addr.sin_addr);

    while (true) {
        Message msg;

        int temp = rand() % 45;
        int hum = rand() % 100;
        const char* air = (rand() % 2) ? "GOOD" : "POOR";

        msg.id = id;
        msg.hum = hum;
        msg.temp = temp;
        strncpy(msg.air, air, sizeof(msg.air));

        cout << "Sensore " << msg.id << ": " << msg.temp << " | " << msg.hum << " | " << msg.air << endl;

        if (sendto(socket, &msg, sizeof(msg), 0, (struct sockaddr*)&central_addr, sizeof(central_addr)) < 0) {
            perror("Errore nell'invio dei dati\n");
            continue;
        }

        sleep(3);
    }

}


int main(int argc, char* argv[]) {

    srand(time(0));

    if (argc != 4) {
        cout << "Usage: " << argv[0] << " <id> <local port> <central node port>\n";
        return -1;
    }

    int id = stoi(argv[1]);
    int port = stoi(argv[2]);
    int central_port = stoi(argv[3]);

    int sockfd;
    struct sockaddr_in address;
    socklen_t len = sizeof(address);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);

    if (bind(sockfd, (struct sockaddr*)&address, len) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }
    cout << "Sensore " << id << " avviato\n";

    function(sockfd, central_port);

    close(sockfd);
    return 0;

}
