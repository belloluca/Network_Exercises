#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fstream>
#include <map>
#include <mutex>
#include <thread>

#define PORT 8080
#define BUFFER 1024

using namespace std;

struct User{

    string email;
    string password;
    string ip;
    bool logged = false;
    bool occupied = false;
    int socket = -1;
    int port;

};

map<string, User> Users;
mutex serverMutex;

string rcvMsg(int socket){

    char buffer[BUFFER];
    int n;

    n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati o client disconnesso\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);

}

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

void saveUser(User user){

    ofstream file("users.txt", ios::app);

    if (!file.is_open()){
        cout << "Errore nell'apertura del file\n";
        return;
    }

    file << user.email << " " << user.password << endl;

    file.close();
}

void loadFile(){

    ifstream file("users.txt");
    User user;
    string email, password;

    if (!file.is_open()){
        cout << "Errore nell'apertura del file\n";
        return;
    }

    while (file >> email >> password){

        user.email = email;
        user.password = password;
        user.socket = -1;
        user.occupied = false;
        user.logged = false;
        user.ip = "";
        user.port = 0;

        Users[email] = user;

    }

    file.close();

}

void userList(string const email, int socket){

    string list = "Utenti online:\n";
    bool found = false;

    for (auto const pair : Users){
        User const user = pair.second;

        if (user.logged && user.email != email){
            list += "-" + user.email + '\n';
            found = true;
        }
    }

    if (!found){
        sendMsg(socket, "Nessun utente online\n");
        return;
    }

    sendMsg(socket, list);

}

void P2P(string const email, int socket){

    sendMsg(socket, "Con chi vuoi parlare?\n");
    string dest = rcvMsg(socket);

    while (dest.empty()){
        sendMsg(socket, "Inserisci l'email di un utente\n");
        dest = rcvMsg(socket);
    }

    bool exist = false;
    bool available = false;

    string senderIp, senderEmail;
    int senderUdpPort;
    int senderSocket;

    string destIp, destEmail;
    int destUdpPort;
    int destSocket;

    {
        lock_guard<mutex> lock(serverMutex);

        if (Users.find(dest) != Users.end()){

            exist = true;

            if (Users[dest].logged && !Users[dest].occupied){

                available = true;

                Users[email].occupied = true;
                Users[dest].occupied = true;

                senderEmail = Users[email].email;
                senderIp = Users[email].ip;
                senderUdpPort = Users[email].port;
                senderSocket = Users[email].socket;

                destEmail = Users[dest].email;
                destIp = Users[dest].ip;
                destUdpPort = Users[dest].port;
                destSocket = Users[dest].socket;
            }
        }
    }

    if (!exist){
        sendMsg(socket, "Utente non esistente\n");
        return;
    }

    if (!available){
        sendMsg(socket, "Utente non connesso o già occupato\n");
        return;
    }

    sendMsg(senderSocket, "Peer info - IP: " + destIp + ", Porta: " + to_string(destUdpPort));
    sendMsg(destSocket, "Peer info - IP: " + senderIp + ", Porta: " + to_string(senderUdpPort));

}

string manageUser(int socket, string const comand, string clientIp){

    string email, password;
    bool logged = false, registered = false;
    User user;

    if (comand == "REGISTER"){

        sendMsg(socket, "Inserisci l'email:\n");
        email = rcvMsg(socket);

        while (email.empty()){

            sendMsg(socket, "Campo 'email' vuoto\n");

            sendMsg(socket, "Inserisci l'email:\n");
            email = rcvMsg(socket);

        }

        sendMsg(socket, "Inserisci password:\n");
        password = rcvMsg(socket);

        while (password.empty()){

            sendMsg(socket, "Campo 'password' vuoto\n");

            sendMsg(socket, "Inserisci password:\n");
            password = rcvMsg(socket);

        }

        {

            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) == Users.end()){

                user.email = email;
                user.password = password;

                Users[email] = user;
                saveUser(user);

            }else{
                registered = true;
            }

        }

        if (!registered){
            sendMsg(socket, "Utente registrato con successo\n");
            sendMsg(socket, "Comandi disponibili: LOGIN | LIST | P2P\n");
        }else{
            sendMsg(socket, "Utente già registrato\n");
        }

    }
    else if(comand == "LOGIN"){

        bool logged = false, error = false, registered = false;

        sendMsg(socket, "Inserisci l'email:\n");
        email = rcvMsg(socket);

        while (email.empty()){

            sendMsg(socket, "Campo 'email' vuoto\n");

            sendMsg(socket, "Inserisci l'email:\n");
            email = rcvMsg(socket);

        }

        sendMsg(socket, "Inserisci password:\n");
        password = rcvMsg(socket);

        while (password.empty()){

            sendMsg(socket, "Campo 'password' vuoto\n");

            sendMsg(socket, "Inserisci password:\n");
            password = rcvMsg(socket);

        }

        sendMsg(socket, "Inserisci porta UDP:\n");
        string udpPortStr = rcvMsg(socket);
        int udpPort = stoi(udpPortStr);

        {

            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) != Users.end() && !Users[email].logged){

                registered = true;

                if (Users[email].password == password){

                    Users[email].logged = true;
                    Users[email].socket = socket;
                    Users[email].ip = clientIp;
                    Users[email].port = udpPort;

                }else{
                    error = true;
                }

            }

        }

        while (error){

            sendMsg(socket, "Password errata\n");
            sendMsg(socket, "Inserisci password\n");

            password = rcvMsg(socket);

            while (password.empty()){

                sendMsg(socket, "Campo 'password' vuoto\n");

                sendMsg(socket, "Inserisci password:\n");
                password = rcvMsg(socket);

            }
                
            if (Users[email].password == password){

                Users[email].logged = true;
                Users[email].socket = socket;
                
                error = false;
                break;

            }

        }

        if (!registered){
            sendMsg(socket, "Utente non registrato o già connesso\n");
        }
        else if (!error){
            sendMsg(socket, "Utente autenticato\n");

            sendMsg(socket, "Comandi disponibili:  LIST | P2P | END_P2P | QUIT\n");
        }

    }

    return email;

}

void handleClient(int socket, string clientIp){

    sendMsg(socket, "Comandi disponibili: REGISTER | LOGIN | LIST | P2P | QUIT\n");

    string email;

    while (true){

        bool occupato = false;

        {

            lock_guard<mutex> lock(serverMutex);

            if (!email.empty() && Users.find(email) != Users.end() && Users[email].logged && Users[email].occupied){
                occupato = true;            
            }

        }

        /* if (occupato){
            this_thread::sleep_for(chrono::microseconds(200));
            continue;
        } 
        */

        string comand = rcvMsg(socket);

        if (comand == "REGISTER" || comand == "LOGIN"){
            email = manageUser(socket, comand, clientIp);
        }
        else if (comand == "LIST"){

            bool error = false;

            {

                lock_guard<mutex> lock(serverMutex);

                if (email.empty() || Users.find(email) == Users.end() || !Users[email].logged){
                    error = true;
                }

            }

            if (error){
                sendMsg(socket, "Utente non registrato o non autenticato\n");
            }else{
                userList(email, socket);
            }

        }
        else if (comand == "P2P"){

            bool error = false;

            {

                lock_guard<mutex> lock(serverMutex);

                if (email.empty() || Users.find(email) == Users.end() || !Users[email].logged){
                    error = true;
                }

            }

            if (error){
                sendMsg(socket, "Utente non registrato o non autenticato\n");
            }else{
                P2P(email, socket);
            }

        }
        else if (comand == "QUIT"){

            sendMsg(socket, "Arrivederci\n");
            cout << "Utente disconnesso " << email << endl;
            
            {

                lock_guard<mutex> lock(serverMutex);

                Users[email].logged = false;
                Users[email].occupied = false;
                Users[email].socket = -1;

            }

            break;
            
        }
        else if (comand == "END_P2P"){

            lock_guard<mutex> lock(serverMutex);

            if (!email.empty() && Users.find(email) != Users.end()){
                Users[email].occupied = false;
            }

            sendMsg(socket, "Chat P2P terminata\n");
        }else{

            sendMsg(socket, "Comando non idoneo\n");
            continue;

        }

    }

    close(socket);

}

int main(){

    loadFile();

    int server_fd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        perror("Errore nella socket\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0){
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 10);
    cout << "Server in ascolto...\n";

    while (true){

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }
        cout << "Cliet connesso...\n";

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);

        thread t(handleClient, new_sockfd, string(ip));
        t.detach();

    }

    close(server_fd);

    return 0;

}
