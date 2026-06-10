#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <set>

using namespace std;

#define SENSOR_PORT 8080
#define CONTROL_PORT 9090
#define BUFFER 1024

mutex ServerMutex;
set<string> Sensors;

void function(int central_socket, sockaddr_in sensor_addr){

    char buffer[BUFFER];
    socklen_t len = sizeof(sensor_addr);
    string data, msg;
    int n;

    while (true){

        n = recvfrom(central_socket, buffer, BUFFER, 0, (struct sockaddr*)&sensor_addr, &len);
        if (n < 0){
            perror("Errore nella ricezione\n");
            continue;
        }
        buffer[n] = '\0';

        data = string(buffer);

        int p1 = data.find("|");
        int p2 = data.find("|", p1 + 1);
        int p3 = data.find("|", p2 + 1);

        string id = data.substr(0, p1);

        int temp = stoi(data.substr(p1 + 1, p2 - p1 - 1));
        int hum = stoi(data.substr(p2 + 1, p3 - p2 - 1));
        int air = stoi(data.substr(p3 + 1));

        {
            lock_guard<mutex> lock(ServerMutex);

            if (Sensors.find(id) == Sensors.end()){
                
                Sensors.insert(id);

                cout << "Sensore " << id << " registrato" << endl;

            }
        }
        
        cout << buffer << endl;

        if (temp > 30 || air > 70){

            int tcp_socket;
            struct sockaddr_in control_addr;

            tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
            if (tcp_socket < 0){
                perror("Errore nella socket TCP\n");
                return;
            }

            control_addr.sin_family = AF_INET;
            control_addr.sin_port = htons(CONTROL_PORT);
            inet_pton(AF_INET, "127.0.0.1", &control_addr.sin_addr);

            if ((connect(tcp_socket, (struct sockaddr*)&control_addr, sizeof(control_addr))) < 0){
                perror("Errore di connessione con il Control Node\n");
                return;
            }

            msg = "ALLARME RILEVATO DAL SENSORE: " + id + " --> Temperatura: " + to_string(temp) + " | Qualità dell'aria: " + to_string(air) + '\n';

            if ((send(tcp_socket, msg.c_str(), msg.size(), 0)) < 0){
                perror("Errore nell'invio dell'allarme\n");
                continue;
            }

            close(tcp_socket);

        }

    }
    
    close(central_socket);

}

int main(){

    int central_socket;
    struct sockaddr_in central_addr, sensor_addr;

    central_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (central_socket < 0){
        perror("Errore nella socket");
        return -1;
    }

    central_addr.sin_family = AF_INET;
    central_addr.sin_port = htons(SENSOR_PORT);
    central_addr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(central_socket, (struct sockaddr*)&central_addr, sizeof(central_addr))) < 0){
        perror("Errore nel bind");
        return -1;
    }
    cout << "Central in ascolto dei sensori...\n";

    thread t(function, central_socket, central_addr);
    t.join();    

}