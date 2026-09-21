#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <mutex>
#include <set>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>

using namespace std;

mutex serverMutex;
set<int> Sensors;

int control_port;

#pragma pack(push, 1)
struct Message {
    int id;
    int temp;
    int hum;
    char air[5];
};
#pragma pack(pop)

void send_alarm(Message alarm) {

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Errore nella socket TCP\n");
        return;
    }

    struct sockaddr_in control_addr;
    socklen_t len = sizeof(control_addr);

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(control_port);
    inet_pton(AF_INET, "127.0.0.1", &control_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&control_addr, len) < 0) {
        perror("Errore nella connessione con il COntrol Node\n");
        return;
    }

    if (send(sockfd, &alarm, sizeof(alarm), 0) < 0) {
        perror("Errore nell'invio dell'allarme\n");
        return;
    }

    cout << "ALLARME INVIATO" << endl;

    close(sockfd);

}

void function(int socket, sockaddr_in address) {

    socklen_t len = sizeof(address);

    Message receive;

    while (true) {
        
        int n = recvfrom(socket, &receive, sizeof(receive), 0, (struct sockaddr*)&address, &len);
        if (n <= 0) {
            perror("Errore nella ricezione dei dati\n");
            continue;
        }
        
        cout << "Sensore " << receive.id << ": " << receive.temp << " | " << receive.hum << " | " << receive.air << endl;

        if (receive.temp > 30 || (strcmp(receive.air, "POOR") == 0)) {
            cout << endl << "ALLARME RILEVATO" << endl;
            send_alarm(receive);
        }

    }

}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <local port> <control port>\n";
        return -1;
    }

    int port = stoi(argv[1]);
    control_port = stoi(argv[2]);

    int sensor_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sensor_sock < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in address;
    socklen_t len = sizeof(address);

    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(sensor_sock, (struct sockaddr*)&address, len) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }
    cout << "Central Node in ascolto\n";

    function (sensor_sock, address);
    

    close(sensor_sock);
    return 0;

}