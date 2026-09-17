#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <map>

using namespace std;

#define BUFFER 1024

struct Sensor {
    int id;
    int socket;
};

mutex serverMutex;
map<int, Sensor> Sensors;

int control_port;
int control_socket;
struct sockaddr_in control_addr;

string rcvMsg(int socket) {
    char buffer[BUFFER];
    
    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0) {
        perror("Errore nella ricezione dei dati\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);
}

void sendMsg(int socket, string msg) {
    if (send(socket, msg.c_str(), msg.size(), 0) < 0) {
        perror("Errore nell'invio dei dati\n");
        return;
    }
}

void Control() {
    string msg;
    while (true) {
        msg = rcvMsg(control_socket);

        if (msg.find("STOP ALARM") != string::npos) {
            int p1 = msg.find(":");
            int id = stoi(msg.substr(p1 + 2));

            int sockfd;

            {
                lock_guard<mutex> lock(serverMutex);

                sockfd = Sensors[id].socket;
            }

            sendMsg(sockfd, "STOP ALARM");
            cout << "Sensore " << id << " sbloccato\n";
        }
    }
    close(control_socket);
}

void send_alarm(string alarm) {

    socklen_t len = sizeof(control_addr);

    sendMsg(control_socket, alarm);
    cout << "ALLARME INVIATO\n";

}

void function(int socket) {
    Sensor sensor;

    while (true) {
        string data = rcvMsg(socket);

        cout << data << endl;

        int p1 = data.find(" ");
        int p2 = data.find(":");

        int id = stoi(data.substr(p1 + 1, p2 - p1 - 1));

        {
            lock_guard<mutex> lock(serverMutex);

            if (Sensors.find(id) == Sensors.end()) {
                sensor.id = id;
                sensor.socket = socket;

                Sensors[id] = sensor;
                cout << "Sensore " << id << " registrato\n";
            }
        }

        p1 = data.find(" ", p2);
        p2 = data.find(" ", p1 + 1);

        int temp = stoi(data.substr(p1 + 1, p2 - p1 - 1));
        
        p1 = data.find(" ", p2 + 1);
        p2 = data.find(" ", p1 + 1);

        int hum = stoi(data.substr(p1 + 1, p2 - p1 - 1));

        p1 = data.find(" ", p2 + 1);

        string air = data.substr(p1 + 1);

        if (temp > 30 || air == "POOR") {
            cout << endl << "ALLARME RILEVATO" << endl;
            send_alarm(data);
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

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 10);
    cout << "Central Node in ascolto\n";

    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(control_port);
    inet_pton(AF_INET, "127.0.0.1", &control_addr.sin_addr);

    if (connect(control_socket, (struct sockaddr*)&control_addr, len) < 0) {
        perror("Errore nella connessione con il control node\n");
        return -1;
    }
    cout << "Connesso al Control Node\n";

    thread control(Control);
    control.detach();

    int new_sockfd;
    while (true) {
        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0) {
            perror("Errore nella connessione\n");
            continue;
        }
        cout << "Sensore connesso\n";

        thread sensor(function, new_sockfd);
        sensor.detach();
    }
    
    close(server_fd);
    return 0;
}