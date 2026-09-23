#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <mutex>
#include <set>
#include "protocol.h"

using namespace std;

set<int> Sensors;
mutex serverMutex;

int server_fd;
struct sockaddr_in sensor_addr;

int sockfd;
struct sockaddr_in control_addr;

Message rcvMsg() {
    Message msg;
    socklen_t len = sizeof(sensor_addr);

    int n = recvfrom(server_fd, &msg, sizeof(msg), 0, (struct sockaddr*)&sensor_addr, &len);
    if (n != sizeof(msg)) {
        perror("Errore nella ricezione dei messaggi");
    }

    return msg;
}

void sendAlarm(Message msg) {

    if (send(sockfd, &msg, sizeof(msg), 0) < 0) {
        perror("Errore nell'invio dei dati al Control Node\n");
        return;
    }
    cout << "Allarme inviato" << endl;

}

void function() {
    Message msg;

    while (true) {
        msg = rcvMsg();

        {
            lock_guard<mutex> lock(serverMutex);

            if (Sensors.find(msg.id) == Sensors.end()) {
                Sensors.insert(msg.id);
                cout << "Sensore " << msg.id << " registrato" << endl; 
            }
        }

        cout << "Sensore " << msg.id << ": " << msg.temp << " | " << msg.hum << " | " << msg.air << endl;

        if (msg.temp > 30 || strcmp(msg.air, "POOR") == 0) {
            cout << "---ALLARME RILEVATO---" << endl;
            sendAlarm(msg);
        }
    }

}

int main(int argc, char* argv[]) {

    if (argc != 3) {
        cout << "usage: " << argv[0] << " <local port> <control port>\n";
        return -1;
    }

    int port = stoi(argv[1]);
    int control_port = stoi(argv[2]);

    server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket UDP\n");
        return -1;
    }

    sensor_addr.sin_family = AF_INET;
    sensor_addr.sin_port = htons(port);
    sensor_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&sensor_addr, sizeof(sensor_addr)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }
    cout << "Central Node in ascolto\n";

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Errore nella socket TCP\n");
        return -1;
    }

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(control_port);
    inet_pton(AF_INET, "127.0.0.1", &control_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&control_addr, sizeof(control_addr)) < 0) {
        perror("Errore nella connessione con il Control Node\n");
        return -1;
    }
    cout << "Connesso al Control Node\n";

    function();

    close(server_fd);
    close(sockfd);
    return 0;

}