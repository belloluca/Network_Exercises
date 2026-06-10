#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <map>
#include <fstream>

using namespace std;

#define PORT 8080
#define BUFFER 1024

struct User{

    string email;
    string password;
    string ip;
    string currentPeer;
    int port = 0;
    int socket = -1;
    bool logged = false;
    bool chatting = false;

};

map<string, User>  Users;
mutex ServerMutex;

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio del messaggio\n");
        return;
    }

}

string recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);

}

void saveUser(User user){

    ofstream file("users.txt", ios::app);

    if (!file.is_open()){

        cout << "Eerrore nell'apertura del file\n";
        return;

    }

    file << user.email << " " << user.password << endl;

    file.close();

}

void loadFile(){

    ifstream file("users.txt");

    if (!file.is_open()){
        cout << "Errore nell'apertura del file\n";
    }

    string email, password;

    while (file >> email >> password){


            User user;
            user.email = email;
            user.password = password;
            user.logged = false;
            user.chatting = false;
            user.socket = -1;
            user.port = 0;

            Users[email] = user;
    }

}

void chat(string user1, string user2){

    string port1 = to_string(Users[user1].port);
    string port2 = to_string(Users[user2].port);

    sendMsg(Users[user1].socket, "Informazioni di " + Users[user2].email + " | " + "Porta: " + port2 + ", Indirizzo IP: " + Users[user2].ip);

    sendMsg(Users[user2].socket, "Informazioni di " + Users[user1].email + " | " + "Porta: " + port1 + ", Indirizzo IP: " + Users[user1].ip);

    return;



}

void handleClient(int socket, string ip){

    string comand, email, password;
    User user;

    sendMsg(socket, "Comandi disponibili: REGISTER | LOGIN | CHAT\n");

    while (true){

        comand = recvMsg(socket);

        if (comand == "REGISTER"){

            bool registered = false;

            sendMsg(socket, "Inserisci email:\n");
            email = recvMsg(socket);

            sendMsg(socket, "Inserisci password:\n");
            password = recvMsg(socket);

            {

                lock_guard<mutex> lock(ServerMutex);

                if (Users.find(email) == Users.end()){

                    user.email = email;
                    user.password = password;
                    Users[email] = user;

                    saveUser(user);

                }else{
                    registered = true;
                }

            }

            if (registered){
                sendMsg(socket, "Utente già registrato\n");
            }else{
                sendMsg(socket, "Registrazione avvenuta con successo\n");

                sendMsg(socket, "Comandi disponibili: LOGIN | CHAT\n");
            }


        }
        else if (comand == "LOGIN"){

            bool registered = false, error = false;

            sendMsg(socket, "Inserisci email:\n");
            email = recvMsg(socket);

            sendMsg(socket, "Inserisci password:\n");
            password = recvMsg(socket);

            {

                lock_guard<mutex> lock(ServerMutex);

                if (Users.find(email) != Users.end() && !Users[email].logged){
                    
                    registered = true;

                    if (Users[email].password == password){

                        Users[email].logged = true;
                        Users[email].socket = socket;
                        Users[email].ip = ip;

                        sendMsg(socket, "Inserisci la tua porta UDP:\n");
                        string portStr = recvMsg(socket);

                        Users[email].port = stoi(portStr);

                        sendMsg(socket, "Comandi disponibili: CHAT\n");

                    }else{
                        error = true;
                    }

                }

            }

            if (!registered){
                sendMsg(socket, "Utente non registrato\n");
                continue;
            }

            while (error){

                sendMsg(socket, "Passoword errata o utente già loggato\n");
                password = recvMsg(socket);

                {

                    lock_guard<mutex> lock(ServerMutex);
                    if (Users[email].password == password){

                        Users[email].logged = true;
                        Users[email].socket = socket;
                        Users[email].ip = ip;
                        error = false;

                        sendMsg(socket, "Inserisci la tua porta UDP:\n");
                        string portStr = recvMsg(socket);

                        Users[email].port = stoi(portStr);

                        sendMsg(socket, "Login effettuato con successo.\n");

                        sendMsg(socket, "Comandi disponibili: CHAT\n");

                    }else{
                        error = true;
                    }

                }

            }

        }
        else if (comand == "CHAT"){

            bool error = false;

            sendMsg(socket, "Inserisci l'utente con cui vuoi parlare:\n");
            string peer = recvMsg(socket);
            
            bool trovato = false;

            {

                lock_guard<mutex> lock(ServerMutex);

                if (Users.find(email) != Users.end() && Users[email].logged){

                    if (Users.find(peer) != Users.end() && Users[peer].logged && !Users[peer].chatting){

                        trovato = true;

                    }

                }else{
                    error = true;
                }

            }

            if (error){
                sendMsg(socket, "Devi prima autenticarti\n");
                continue;
            }

            if (trovato){
                sendMsg(socket, "Utente trovato\n");
                
                {

                    lock_guard<mutex> lock(ServerMutex);

                    Users[email].chatting = true;
                    Users[peer].chatting = true;

                    Users[email].currentPeer = peer;
                    Users[peer].currentPeer = email;

                }
                
                chat(email, peer);
                continue;

            }else{
                sendMsg(socket, "Utente non registrato o offline\n");
            }

        }
        else if (comand == "QUIT"){

            {

                lock_guard<mutex> lock(ServerMutex);

                if (!email.empty() && Users.find(email) != Users.end()) {
                    Users[email].logged = false;
                    Users[email].chatting = false;
                    Users[email].socket = -1;
                    string peer = Users[email].currentPeer;
                    Users[email].currentPeer = "";
                    Users[peer].currentPeer = "";
                    Users[peer].chatting = false;
                }

            }

            sendMsg(socket, "Arrivederci!\n");

            break;

        }else{
            sendMsg(socket, "Comando non disponibile\n");
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
    cout << "Server in ascolto..." << endl;

    while (true){

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

        string ip = string(client_ip);

        thread t(handleClient, new_sockfd, ip);
        t.detach();

    }

    close(server_fd);
    return 0;

}