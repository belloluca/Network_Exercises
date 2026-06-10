#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <set>
#include <mutex>

using namespace std;

#define SENSOR_PORT 8080
#define CONTROL_PORT 9090
#define BUFFER 1024

set<string> Sensors;
mutex serverMutex;

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

string recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati");
        return "";
    }
    buffer[n] = '\0';

    
    return string(buffer);

}

bool AlarmControl(string const element, string const status){

    if (element == "DOOR" && status == "OPEN")
        return true;
    if (element == "WINDOW" && status == "OPEN")
        return true;
    if (element == "MOTION" && status == "DETECTED")
        return true;
    
    return false;

}

string manageAlarm(string const alarm){

    int sockfd;
    struct sockaddr_in control_addr;
    socklen_t len = sizeof(control_addr);
    string receive;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("Errore nella socket per il Control Node\n");
        return "";
    }

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(CONTROL_PORT);
    inet_pton(AF_INET, "127.0.0.1", &control_addr.sin_addr);

    if ((connect(sockfd, (struct sockaddr*)&control_addr, len)) < 0){
        perror("Errore nella connessione con il Control Node\n");
        return "";
    }
    cout << "Connesso al Control Node...\n";

    sendMsg(sockfd, alarm);

    receive = recvMsg(sockfd);
    int pos1 = receive.find(" ");
    int pos2 = receive.find(":");
    string ID = receive.substr(pos1 + 1, pos2 - (pos1 + 1));

    return ID;

}

void handleSensor(int socket){

    string receive, id;
    bool alarm = false;

    while (true){

        receive = recvMsg(socket);

        cout << receive << endl;

        int pos1 = receive.find(" ");
        int pos2 = receive.find("-");
        string ID = receive.substr(pos1 + 1, pos2 - (pos1 + 1));
        Sensors.insert(ID);
        
        int pos3 = receive.find(":");
        string element = receive.substr(pos2 + 1, pos3 - (pos2 + 1));

        string status = receive.substr(pos3 + 2);

        alarm = AlarmControl(element, status);

        if (alarm){

            cout << "Il sensore " << ID << " entra in modalità ALARM" << endl;

            id = manageAlarm(receive);

            if (Sensors.find(id) != Sensors.end()){
                sendMsg(socket, "STOP_ALARM");
                continue;
            }

        }

    }

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
            perror("Errore nella connessione del Sensore\n");
            continue;
        }

        thread t(handleSensor, sensor_socket);
        t.detach();

    }

    close(central_socket);
    return 0;

}