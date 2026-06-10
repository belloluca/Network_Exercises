#include <iostream>
#include <cstring>
#include <string>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>

using namespace std;

#define PORT 9090
#define BUFFER 1024

void saveAlarms(string const alarm){

    fstream file("home_alarm_log.txt", ios::app);

    if (!file.is_open()){
        cout << "Errore nell'aprire il file" << endl;
        return;
    }

    file << alarm << endl;

    file.close();

}

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

void recvMsg(int socket){

    while (true){

        char buffer[BUFFER];
        string receive;

        int n = recv(socket, buffer, BUFFER - 1, 0);
        if (n <= 0){
            perror("Errore nella ricezione dei dati");
            return;
        }
        buffer[n] = '\0';

        cout << buffer << endl;

        saveAlarms(string(buffer));

        receive = string(buffer);
        int pos1 = receive.find(" ");
        int pos2 = receive.find("-");
        string ID = receive.substr(pos1 + 1, pos2 - (pos1 + 1));

        sleep(5);
        sendMsg(socket, "Sensor " + ID + ": " + "STOP_ALARM");

    }


}

int main(){

    int control_socket, central_socket;
    struct sockaddr_in control_addr, central_addr;
    socklen_t len = sizeof(central_addr);

    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket < 0){
        perror("Errore nella socket\n");
        return -1;
    }

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(PORT);
    control_addr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(control_socket, (struct sockaddr*)&control_addr, sizeof(control_addr))) < 0){
        perror("Errore nel bind\n");
        return -1;
    }

    listen(control_socket, 10);
    cout << "Control Node in ascolto...\n";

    while (true){

        central_socket = accept(control_socket, (struct sockaddr*)&central_addr, &len);
        if (central_socket < 0){
            perror("Errore nella connessione del Central Node\n");
            continue;
        }
        cout << "Central Node connesso...\n";

        thread t(recvMsg, central_socket);
        t.detach();

    }

    close(control_socket);
    return 0;


}