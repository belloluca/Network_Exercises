// Bene ma migliorabile, sistemare quando si riceve la richiesta di avvio di una chat
#include <iostream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <fstream>
#include <mutex>
#include <map>

using namespace std;

#define PORT 8080
#define BUFFER 1024

struct User{

    string email;
    string password;
    bool logged = false;
    int socket = -1;

};

mutex ServerMutex;
map<string, User> Users;

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

string recvMsg(int socket){

    char buffer[BUFFER];
    int n;

    n = recv(socket, buffer, BUFFER, 0);
    if (n < 0){
        perror("Errore nella ricezione dei dati\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);

}

void saveFile(User utente){

    ofstream file("users.txt", ios::app);

    if (!file.is_open()){
        cout << "Errore nell'apertura del file\n";
        return;
    }

    file << utente.email << " " << utente.password << endl;

    file.close();

}

void loadFile(){

    ifstream file("users.txt");

    if (!file.is_open()){
        cout << "Errore nell'apertura del file\n";
        return;
    }

    string email, password;

    {

        lock_guard<mutex> lock(ServerMutex);

        while (file >> email >> password){

            User utente;
            utente.email = email;
            utente.password = password;
            utente.logged = false;
            utente.socket = -1;

            Users[email] = utente;

        }

    }

    file.close();

    cout << "Utenti caricati dal file\n";

}

void Inoltro(int socketMittente, int socketDestinatario, string mittente) {

    string msg;

    while (true) {

        msg = recvMsg(socketMittente);

        if (msg.empty() || msg == "QUIT") {

            string avviso = "L'utente " + mittente + " si è disconnesso\n";
            sendMsg(socketDestinatario, avviso);

            close(socketMittente);
            break;
        }

        sendMsg(socketDestinatario, msg);
    }
}

void chat(string utente1, string utente2) {

    int socket1, socket2;

    {
        lock_guard<mutex> lock(ServerMutex);

        socket1 = Users[utente1].socket;
        socket2 = Users[utente2].socket;
    }

    string msg;

    msg = "Comunicazione avviata con l'utente: " + utente1;
    sendMsg(socket2, msg);

    msg = "Comunicazione avviata con l'utente: " + utente2;
    sendMsg(socket1, msg);

    thread t1(Inoltro, socket1, socket2, utente1);
    t1.detach();

    thread t2(Inoltro, socket2, socket1, utente2);
    t2.detach();
}

void handleClient(int socket){

    string msg, receive, email, password;
    User utente;

    msg = "Comandi disponibili: REGISTER | LOGIN | TEXT\n";
    sendMsg(socket, msg);

    while (true){

        receive = recvMsg(socket);

        if (receive == "REGISTER"){

            msg = "Inserisci email: ";
            sendMsg(socket, msg);

            email = recvMsg(socket);

            msg = "Inserisci password: ";
            sendMsg(socket, msg);

            password = recvMsg(socket);
            
            {

                lock_guard<mutex> lock(ServerMutex);

                if (Users.find(email) != Users.end()){
                    msg = "Utente già registrato\n";
                    sendMsg(socket, msg);
                }else{

                    msg = "Utente registrato correttamente\n";

                    utente.email = email;
                    utente.password = password;
                    Users[email] = utente;
                    
                    saveFile(utente);

                }

            }

            sendMsg(socket, msg);

        }
        else if (receive == "LOGIN"){

            msg = "Inserisci email: ";
            sendMsg(socket, msg);

            email = recvMsg(socket);

            msg = "Inserisci password: ";
            sendMsg(socket, msg);

            password = recvMsg(socket);

            {

                lock_guard<mutex> lock(ServerMutex);

                if (Users.find(email) != Users.end()){

                    if (Users[email].password == password){

                        msg = "Accesso avvenuto correttamente\n";
                        sendMsg(socket, msg);

                        Users[email].logged = true;
                        Users[email].socket = socket;

                    }else{
                        msg = "Password errata\n";
                        sendMsg(socket, msg);
                    }

                }else{
                    msg = "Utente non registrato\n";
                    sendMsg(socket, msg);
                }

            }

        }
        else if (receive == "TEXT"){

            bool start = false;

            {

                lock_guard<mutex> lock(ServerMutex);

                if (Users[email].logged){

                    msg = "Con chi vuoi chattare?\n";
                    sendMsg(socket, msg);
                    
                    receive = recvMsg(socket);

                    if (Users.find(receive) != Users.end() && Users[receive].logged){

                        start = true;

                    }else{
                        msg = "Utente non trovato\n";
                        sendMsg(socket, msg);
                    }

                }else{
                    msg = "Devi prima autenticarti\n";
                    sendMsg(socket, msg);
                }

            }

            if (start){

                chat(Users[email].email, Users[receive].email);
                break;
            }

        }
        else if (receive == "QUIT"){

            msg = "Arrivederci\n";
            sendMsg(socket, msg);

            cout << "Utente " << email << " disconnesso\n";

            {
                lock_guard<mutex> lock(ServerMutex);

                close(Users[email].socket);
                Users[email].logged = false;
        
            }

            break;

        }
        // Fare un if (session->logged), e da qui monitorare il comando TEXT
        else{
            msg = "Comando non disponibile\n";
            sendMsg(socket, msg);
        }

    }

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
    cout << "Server avviato...\n";

    while (true){

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }
        cout << "Client connesso" << endl;

        thread t(handleClient, new_sockfd);
        t.detach();

    }

}
