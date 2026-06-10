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
    bool logged = false;
    bool occupied = false;
    int socket = -1;

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

void Deliver(string sender, string receiver, int socket1, int socket2){

    string msg;

    while (true){

        sendMsg(socket1, "Invia messaggio:\n");
        msg = rcvMsg(socket1);

        if (msg == "EXIT CHAT"){
            sendMsg(socket1, "Arrivederci\n");
            cout << "Utente: " << sender << " disconnesso\n";
            sendMsg(socket2, "L'utente " + sender + " si è disconnesso\n");

            {

                lock_guard<mutex> lock(serverMutex);

                Users[sender].occupied = false;
                Users[receiver].occupied = false;

            }

            break;
        }

        sendMsg(socket2, sender + ": " + msg);

    }

}

void sendTask(string const sender, int socket){

    sendMsg(socket, "A chi vuoi mandare la Task?\n");
    string dest = rcvMsg(socket);

    while (dest.empty()){
        sendMsg(socket, "Inserisci l'email di un utente\n");
        dest = rcvMsg(socket);
    }

    bool exist = false, logged = false;
    int socket2;

    {

        lock_guard<mutex> lock(serverMutex);

        if (Users.find(dest) != Users.end()){

            exist = true;

            if (Users[dest].logged){

                logged = true;

            }

        }

    }

    if (exist){
        
        if (logged){

        {

            lock_guard<mutex> lock(serverMutex);

            socket2 = Users[dest].socket;
            
        }

        }else{
            sendMsg(socket, "Utente non connesso\n");
            return;
        }

    }else{
        sendMsg(socket, "Utente non esistente\n");
        return;
    }

    string msg;

    sendMsg(socket, "Invia Task:\n");
    msg = rcvMsg(socket);

    if (msg == "QUIT"){
        sendMsg(socket, "Arrivederci\n");
        cout << "Utente: " << sender << " disconnesso\n";
        close(socket); 
    }

    sendMsg(socket2, "Task assegnata da " + sender + ": " + msg);

}

void Message(string const email, int socket){

    sendMsg(socket, "Con chi vuoi parlare?\n");
    string dest = rcvMsg(socket);

    while (dest.empty()){
        sendMsg(socket, "Inserisci l'email di un utente\n");
        dest = rcvMsg(socket);
    }

    bool exist = false, logged = false;

    {

        lock_guard<mutex> lock(serverMutex);

        if (Users.find(dest) != Users.end()){

            exist = true;

            if (Users[dest].logged && !Users[dest].occupied){

                logged = true;

                Users[email].occupied = true;
                Users[dest].occupied = true;

            }

        }

    }

    if (exist){
        
        if (logged){

            int socket1, socket2;

        {

            lock_guard<mutex> lock(serverMutex);

            socket1 = Users[email].socket;
            socket2 = Users[dest].socket;
            
        }

        sendMsg(socket1, "Chat avviata con " + dest + "\n");
        sendMsg(socket2, "Chat avviata con " + email + "\n");

        thread chatting1(Deliver,email, dest, socket1, socket2);
        thread chatting2(Deliver, dest, email, socket2, socket1);

        chatting1.detach();
        chatting2.detach();

        }else{
            sendMsg(socket, "Utente non connesso\n");
        }

    }else{
        sendMsg(socket, "Utente non esistente\n");
    }

}

string manageUser(int socket, string const comand){

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
            sendMsg(socket, "Comandi disponibili: LOGIN | LIST | MSG | TASK\n");
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

        {

            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) != Users.end() && !Users[email].logged){

                registered = true;

                if (Users[email].password == password){

                    Users[email].logged = true;
                    Users[email].socket = socket;

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

            sendMsg(socket, "Comandi disponibili:  LIST | MSG | TASK | QUIT\n");
        }

    }

    return email;

}

void handleClient(int socket){

    sendMsg(socket, "Comandi disponibili: REGISTER | LOGIN | LIST | MSG | TASK | QUIT\n");

    string email;

    while (true){

        bool occupato = false;

        {

            lock_guard<mutex> lock(serverMutex);

            if (!email.empty() && Users.find(email) != Users.end() && Users[email].logged && Users[email].occupied){
                occupato = true;            
            }

        }

        if (occupato){
            this_thread::sleep_for(chrono::microseconds(200));
            continue;
        }

        string comand = rcvMsg(socket);

        if (comand == "REGISTER" || comand == "LOGIN"){
            email = manageUser(socket, comand);
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
        else if (comand == "MSG"){

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
                Message(email, socket);
            }

        }
        else if (comand == "TASK"){


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
                sendTask(email, socket);
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

        thread t(handleClient, new_sockfd);
        t.detach();

    }

    close(server_fd);

    return 0;

}