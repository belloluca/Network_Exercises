#include <iostream>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include <map>
#include <thread>
#include <unistd.h>
#include <stdlib.h>
#include <fstream>

using namespace std;

#define PORT 8080
#define BUFFER 1024

struct User{

    string email;
    string password;
    bool logged = false;
    bool chatting = false;
    int socket = -1;
    int port;
    string ip;

};

map<string, User> Users;
mutex serverMutex;

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0) < 0)){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

string recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati o client disconnesso\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);

}

void saveUser(User const user){

    ofstream file("users.txt", ios::app);

    if (!file.is_open()){
        cout << "Impossibile aprire il file" << endl;
        return;
    }

    file << user.email << " " << user.password << endl;

    file.close();

}

void loadFile(){

    ifstream file("users.txt");
    string email, password;
    User user;

    if (!file.is_open()){
        cout << "Impossibile aprire il file" << endl;
        return;
    }

    while (file >> email >> password){
        
        user.email = email;
        user.password = password;
        user.socket = -1;
        user.chatting = false;
        user.logged = false;
        user.ip = "";
        user.port = 0;

        Users[email] = user;

    }

    file.close();

}

void chatP2P(int socket1, string const sender){

    string receiver;
    string send_ip, rec_ip;
    int port1, port2;
    int socket2;
    bool found = false, logged = false;

    {

        lock_guard<mutex> lock(serverMutex);

        if (Users[sender].logged && !Users[sender].chatting){
            logged = true;
        }

    }

    if (!logged){
        sendMsg(socket1, "Devi prima autenticarti");
        return;
    }        

    sendMsg(socket1, "Con chi vuoi parlare?");
    receiver = recvMsg(socket1);

    {

        lock_guard<mutex> lock(serverMutex);

        if (Users[sender].logged && !Users[sender].chatting){

            logged = true;

            if (Users.find(receiver) != Users.end() && Users[receiver].logged){

                port1 = Users[sender].port;
                port2 = Users[receiver].port;

                send_ip = Users[sender].ip;
                rec_ip = Users[receiver].ip;

                Users[sender].chatting = true;
                Users[receiver].chatting = true;

                socket2 = Users[receiver].socket;

                found = true;

            }

        }

    }


    if (found){
        sendMsg(socket1, "Indirizzo IP di " + receiver + ": " + rec_ip + ", Porta P2P di " + receiver + ": " + to_string(port2) + "\n");

        sendMsg(socket2, "Indirizzo IP di " + sender + ": " + send_ip + ", Porta P2P di " + sender + ": " + to_string(port1) + "\n");
    }else{
        sendMsg(socket1, "Utente non disponibile");
    }

    
}

void userList(int socket, string const email){

    string list = "Lista degli utenti online:\n";
    bool found = false;

    lock_guard<mutex> lock(serverMutex);

    for (auto const pair : Users){

        User const user = pair.second;

        if (user.logged){
            if (user.email != email){
                list += "- " + user.email + '\n';
                found = true;
            }
        }

    }

    if (found){
        sendMsg(socket, list);
    }else{
        sendMsg(socket, "Nessun utente online");
    }
}

string manageUser(int socket, string const comand, string client_addr){

    string email, password;
    User user;

    sendMsg(socket, "Inserisci l'email: ");
    email = recvMsg(socket);

    while (email.empty()){

        sendMsg(socket, "Il campo dell'email è vuoto");
        sendMsg(socket, "Inserisci l'email: ");

        email = recvMsg(socket);

    }

    sendMsg(socket, "Inserisci password: ");
    password = recvMsg(socket);

    while (password.empty()){

        sendMsg(socket, "Il campo della password è vuoto");
        sendMsg(socket, "Inserisci password: ");

        password = recvMsg(socket);

    }

    bool registered = false;

    if (comand == "REGISTER"){

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

        if (registered){
            sendMsg(socket, "Utente già registrato");
        }else{
            sendMsg(socket, "Utente registrato con successo\n");
            sendMsg(socket, "Comandi disponibili: LOGIN | LIST | CHAT | QUIT");
        }

    }
    else if (comand == "LOGIN"){

        string P2P;
        bool logged = false, error = false, registered = false;

        
        sendMsg(socket, "Inserisci porta P2P: ");
        P2P = recvMsg(socket);

        while (P2P.empty()){

            sendMsg(socket, "Il campo della porta è vuoto\n");
            sendMsg(socket, "Inserisci porta: ");

            P2P = recvMsg(socket);

        }

        {

            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) != Users.end()){

                registered = true;

                if (!Users[email].logged){

                    if (Users[email].password == password){

                        Users[email].socket = socket;
                        Users[email].logged = true;
                        Users[email].ip = client_addr;
                        Users[email].port = stoi(P2P);
                        
                    }else{
                        error = true;
                    }

                }else{
                    logged = true;
                }

            }
        }

        if (registered){
            if (!logged){
                while (error){

                    sendMsg(socket, "Passoword errata");
                    
                    sendMsg(socket, "Inserisci password: ");
                    password = recvMsg(socket);

                    while (password.empty()){

                        sendMsg(socket, "Il campo della password è vuoto");
                        sendMsg(socket, "Inserisci password: ");

                        password = recvMsg(socket);

                    }

                    {

                        lock_guard<mutex> lock(serverMutex);

                        if (Users[email].password == password){

                            Users[email].socket = socket;
                            Users[email].logged = true;
                            Users[email].ip = client_addr;
                            Users[email].port = stoi(P2P);
                            
                            error = false;

                        }

                    }

                }
                sendMsg(socket, "Utente autenticato\n");
                sendMsg(socket, "Comandi disponibili: LIST | CHAT | QUIT");
            }else{
                sendMsg(socket, "Utente già connesso");
            }
        }else{
            sendMsg(socket, "Utente non registrato");
        }

    }

    return email;

}

void handleClient(int socket, string client_addr){

    sendMsg(socket, "Comandi disponibili: REGISTER | LOGIN | LIST | CHAT | QUIT");

    string email;

    while (true){

        string comand = recvMsg(socket);

        bool occupied = false;

        {

            lock_guard<mutex> lock(serverMutex);

            if (!email.empty() && Users.find(email) != Users.end() && Users[email].logged && Users[email].chatting)
                occupied = true;
        }

        if (occupied){
            this_thread::sleep_for(chrono::milliseconds(200));
            continue;
        }

        if (comand == "REGISTER" || comand == "LOGIN"){
            email = manageUser(socket, comand, client_addr);
        }
        else if (comand == "LIST"){

            bool error = false;
        
            {

                lock_guard<mutex> lock(serverMutex);

                if (!Users[email].logged || Users[email].chatting)
                    error = true;

            }

            if (!error){
                userList(socket, email);
            }else{
                sendMsg(socket, "Errore nella visualizzazione della lista. Controlla se hai effettuato il login e non sei in una chat");
            }

        }
        else if (comand == "CHAT"){
            chatP2P(socket, email);
        }
        else if (comand == "QUIT"){

            sendMsg(socket, "Arrivederci!");
            cout << "Client " << email << " disconnesso\n";

            {

                lock_guard<mutex> lock(serverMutex);

                Users[email].logged = false;
                Users[email].port = 0;
                Users[email].chatting = false;
                Users[email].ip = "";
                Users[email].socket = -1;

            }

            break;
        }else{
            sendMsg(socket, "Comando non idoneo");
        }

    }

    close(socket);

}

int main(){

    loadFile();

    int server_fd, new_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(server_addr);

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

        new_socket = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_socket < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }
        cout << "Client connesso...\n";

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);
        string client_ip = string(ip);

        thread t(handleClient, new_socket, client_ip);
        t.detach();

    }

    close(server_fd);
    
    return 0;

}