#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <time.h>
#include <thread>
#include <mutex>
#include <set>

#define BUFFER 1024
#define UDP_PORT 8080
#define TCP_PORT 9090

using namespace std;

mutex ServerMutex;
set<string> Sensors;

void sendMsgTCP(int socket, string msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

string recvMsgUDP(int socket, sockaddr_in server_addr){

    

        char buffer[BUFFER];
        socklen_t len = sizeof(server_addr);

        int n = recvfrom(socket, buffer, BUFFER - 1, 0, (struct sockaddr*)&server_addr, &len);
        if (n <= 0){
            perror("Errore nella ricezione di messaggi o Sensor Node disconnesso\n");
            return "";
        }
        buffer[n] = '\0';

        cout << "Messaggio ricevuto: " << buffer << endl;

        return string(buffer);

    

}

void control(string const msg){

    int sockfd;
    struct sockaddr_in control_addr;
    socklen_t len = sizeof(control_addr);

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        perror("Errore nella socket TCP\n");
        return;
    }

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &control_addr.sin_addr);

    if ((connect(sockfd, (struct sockaddr*)&control_addr, len) < 0)){
        perror("Errore nella connessione con il Control Node\n");
        return;
    }
    cout << "Connesso al Control Node...\n";

    sendMsgTCP(sockfd, "ALLARME | " + msg);

    close(sockfd);

}

void function(int socket, sockaddr_in sensor_addr){

    string receive;

    while (true){

        receive = recvMsgUDP(socket, sensor_addr);

        string id = receive.substr(8, receive.find(":") - 8);
        int pos1 = receive.find(":");
        int pos2 = receive.find(":", pos1 + 1);
        int pos3 = receive.find("|");
        string status = receive.substr(pos2 + 2, pos3 - (pos2 + 3));

        pos1 = receive.find(":", pos3);
        pos2 = receive.find("|", pos1);
        int time = stoi(receive.substr(pos1 + 2, pos2 - (pos1 + 2)));

        pos3 = receive.find(":", pos2);
        int battery = stoi(receive.substr(pos3 + 2));

        if (Sensors.find(id) == Sensors.end()){
            cout << "Sensore " << id << " registrato" << endl;

            {
                lock_guard<mutex> lock(ServerMutex);

                Sensors.insert(id);
            
            }
        }

        if (status == "OCCUPIED" && time > 40 || battery < 20){

            control(receive);

        }

    }

}



int main(){

    int central_socket, sensor_socket;
    struct sockaddr_in central_addr, sensor_addr;
    socklen_t len = sizeof(central_addr);

    central_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (central_socket < 0){
        perror("Errore nella socket UDP\n");
        return -1;
    }

    central_addr.sin_family = AF_INET;
    central_addr.sin_port = htons(UDP_PORT);
    central_addr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(central_socket, (struct sockaddr*)&central_addr, sizeof(central_addr))) < 0){
        perror("Errore nel bind\n");
        return -1;
    }

    thread t(function, central_socket, central_addr);
    t.join();

    close(central_socket);
    close(sensor_socket);

    return 0;
}