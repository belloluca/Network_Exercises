#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <thread>
#include <mutex>
#include <set>
#include <map>

using namespace std;

#define SENSOR_PORT 8080
#define CONTROL_PORT 9090

#define BUFFER 1024

map<string, int> SensorsMap;
mutex CentraMutex;


void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

string recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER, 0);
    if (n < 0){
        perror("Errore nella ricezione del messaggio\n");
        return ""; 
    }
    buffer[n] = '\0';

    return string(buffer);

}

string sendAlarm(string const msg, int sockfd){

    string receive;

    sendMsg(sockfd, msg);

    receive = recvMsg(sockfd);
    
    return receive;

}

void function(int sensor_socket){

    string msg, receive;

    int sockfd;
    struct sockaddr_in control_addr;
    socklen_t len = sizeof(control_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("Errore nella socket\n");
        return;
    }

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(CONTROL_PORT);
    inet_pton(AF_INET, "127.0.0.1", &control_addr.sin_addr);

    if((connect(sockfd, (struct sockaddr*)&control_addr, len)) < 0){
        perror("Errore nella connessione con il Control Node...\n");
        return;
    }
    
    cout << "Connesso al Contro Node..." << endl;

    while (true){

        receive = recvMsg(sensor_socket);

        int pos = receive.find(":");
        string ID = receive.substr(7, pos - 7);

        int pos1 = receive.find("|", pos + 1);
        int temp = stoi(receive.substr(pos + 1, pos1 - pos - 1));

        int pos2 = receive.find("|", pos1 + 1);
        int hum = stoi(receive.substr(pos1 + 1, pos2 - pos1 - 1));

        int air = stoi(receive.substr(pos2 + 1));

        {

            lock_guard<mutex> lock(CentraMutex);

            SensorsMap[ID] = sensor_socket;
        
        }

        cout << receive << endl;

        msg = "Sensor " + ID + ": " + to_string(temp) + " | " + to_string(hum) + " | " + to_string(air);

        if (temp > 30 || air > 70){

            string id = sendAlarm(msg, sockfd);

            {
                lock_guard<mutex> lock(CentraMutex);

                if (SensorsMap.find(id) != SensorsMap.end()){

                    int socket = SensorsMap[id];

                    msg = "STOP ALARM";

                    sendMsg(socket, msg);
                }
        
            }

        }

    }

    close(sensor_socket);
    close(sockfd);

}

int main(){

    int central_socket, sensor_socket;
    struct sockaddr_in central_addr, sensor_addr;
    socklen_t len = sizeof(sensor_addr);

    central_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (central_socket < 0){
        perror("Errore nella socket\n");
        return -1;
    }

    central_addr.sin_family = AF_INET;
    central_addr.sin_port = htons(SENSOR_PORT);
    central_addr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(central_socket, (struct sockaddr*)&central_addr, sizeof(central_addr))) < 0){
        perror("Errore nel bind\n");
        return -1;
    }

    listen(central_socket, 10);
    cout << "Central Node in ascolto...\n";

    while (true){

        sensor_socket = accept(central_socket, (struct sockaddr*)&sensor_addr, &len);
        if (sensor_socket < 0){
            perror("Connessione con il sensore fallita...\n");
            continue;
        }
        cout << "Connesso al Sensore...\n";

        thread t(function, sensor_socket);
        t.detach();

    }

    close(central_socket);
    return 0;

}