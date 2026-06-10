#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <cstring>
#include <mutex>
#include <set>
#include <fstream>

#define PORT 9090
#define BUFFER 1024

using namespace std;

string recvMsg(int socket){

    char buffer[BUFFER];
    int n;

    n = recv(socket, buffer, BUFFER, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati o Central Node disconnesso\n");
        return "";
    }
    buffer[n] = '\0';

    cout << "Messaggio ricevuto: " << buffer << endl;

    return string(buffer);

}

void saveFile(string alarm){

    ofstream file("parking_log.txt", ios::app);

    if (!file.is_open()){
        cout << "Errore nell'apertura del file" << endl;
    }

    file << alarm << endl;

    file.close();

}

void function(int socket){

    string receive = recvMsg(socket);
    receive = receive.substr(11);

    saveFile(receive);

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

    listen(control_socket, 1);
    cout << "Control Node in ascolto...\n";

    while (true){

        central_socket = accept(control_socket, (struct sockaddr*)&central_addr, &len);
        if (central_socket < 0){
            perror("Errore di connessione con il Central Node...\n");
            return -1;
        }

        function(central_socket);

        close(central_socket);

    }

    close(control_socket);
    return 0;

}   