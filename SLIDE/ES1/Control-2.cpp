#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <mutex>

using namespace std;

#define PORT 9090
#define BUFFER 1024

mutex fileMutex;

int main(){

    int control_socket, central_socket;
    struct sockaddr_in control_addr, central_addr;
    socklen_t len = sizeof(central_addr);
    ofstream logfile("alarms.log", ios::app);
    int n;
    char buffer[BUFFER];

    control_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (control_socket < 0){
        perror("Errore socket\n");
        return -1;
    }

    control_addr.sin_family = AF_INET;
    control_addr.sin_port = htons(PORT);
    control_addr.sin_addr.s_addr = INADDR_ANY;

    if((bind(control_socket, (struct sockaddr*)&control_addr, sizeof(control_addr))) < 0){
        perror("Errore nel bind\n");
        return -1;
    }

    listen(control_socket, 10);
    cout << "Control Node in ascolto...\n";

    while (true){

        central_socket = accept(control_socket, (struct sockaddr*)&central_addr, &len);
        if (central_socket < 0){
            perror("Errore nella connessione1n");
            return -1;
        }

        n = recv(central_socket, buffer, BUFFER, 0);
        if (n < 0){
            perror("Errore nella ricezione\n");
            continue;
        }
        buffer[n] = '\0';

        cout << "|ALLARME RICEVUTO| " << buffer << endl;

        logfile << buffer << endl;

        logfile.close();

        close(central_socket);

    }

    close(control_socket);

    return 0;

}