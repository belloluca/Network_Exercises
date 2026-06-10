#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <thread>

using namespace std;

#define PORT 9090
#define BUFFER 1024

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

void saveAlarm(string const alarmMsg){

    ofstream file("alarm_log.txt", ios::app);

    if (!file){
        perror("Errore apertura file LOG");
        return;
    }

    file << alarmMsg << endl;

    file.close();

}

void function(int socket){

    string receive, msg;

    while (true){

        receive = recvMsg(socket);

        cout << "Messaggio ricevuto: " << receive << endl;

        saveAlarm(receive);

        int pos = receive.find(":");
        string id = receive.substr(7, pos - 7);

        sleep(5);

        msg = id;
        sendMsg(socket, msg);

    }

    close(socket);

}

int main(){

    int central_socket, control_socket;
    struct sockaddr_in central_addr, control_addr;
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

    listen(control_socket, 1);
    cout << "Control Node avviato...\n";

    central_socket = accept(control_socket, (struct sockaddr*)&central_addr, &len);
    if (central_socket < 0){
        perror("Errore nella connessione con il Central Node...\n");
        return -1;
    }

    
    function(central_socket);

    close(central_socket);
    close(control_socket);
    return 0;

}
