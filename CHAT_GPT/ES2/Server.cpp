#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <map>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fstream>
#include <queue>
#include <chrono>

#define PORT 8080
#define BUFFER 1024

using namespace std;

struct User{

    string email;
    string password;
    bool loggend = false;
    bool waiting = false;
    bool inGame = false;
    int socket = -1;

};

map<string, User> Users;
queue<string> waitingList;
mutex serverMutex;

void sendMsg(int socket, string const msg){

    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio dei dati\n");
        return;
    }

}

string recvMsg(int socket){

    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER, 0);
    if (n <= 0){
        perror("Errore nella ricezione dei dati\n");
        return "";
    }
    buffer[n] = '\0';

    return string(buffer);

}

void saveUser(User user){

    ofstream file("player.txt", ios::app);

    if (!file.is_open()){
        cout << "Errore nella lettura del file\n";
        return;
    }

    file << user.email << " " << user.password << endl;

    file.close();

}

void loadFile(){

    ifstream file("player.txt");
    string email, password;
    User user;

    if (!file.is_open()){
        cout << "Errore nella apertura del file\n";
        return;
    }

    {

        lock_guard<mutex> lock(serverMutex);

        while (file >> email >> password){

            user.email = email;
            user.password = password;
            user.inGame = false;
            user.waiting = false;
            user.loggend = false;
            user.socket = -1;

            Users[email] = user;

        }
    }

    file.close();

}

string comands(int socket, string comand){

    User user;
    string email, password;
    bool registered = false, logged = false, error = false;

    sendMsg(socket, "Inserisci email:\n");
    email = recvMsg(socket);

    sendMsg(socket, "Inserisci password:\n");
    password = recvMsg(socket);

    if (comand == "REGISTER"){

        {

            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) == Users.end() && !email.empty()){

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
            sendMsg(socket, "Comandi disponibili --> LOGIN | PLAY\n");
        }else{
            sendMsg(socket, "Utente già registrato\n");
        }

    }
    else if (comand == "LOGIN"){

        {

            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) != Users.end() && !email.empty()){

                registered = true;

                if (!Users[email].loggend){

                    if (Users[email].password == password){

                        logged = true;

                        Users[email].loggend = true;
                        Users[email].socket = socket;

                    }else{

                        error = true;

                    }

                }

            }

        }

        while (error){

            sendMsg(socket, "Password errata\n");

            sendMsg(socket, "Inserisci password:\n");
            password = recvMsg(socket);

            {

                lock_guard<mutex> lock(serverMutex);

                if (Users[email].password == password){

                    logged = true;

                    Users[email].loggend = true;

                    error = false;

                }                
            }

        }

        if (registered){

            if (logged){

                sendMsg(socket, "Utente autenticato\n");
                sendMsg(socket, "Comandi disponibili --> PLAY\n");

            }else{
                sendMsg(socket, "Utente già autenticato\n");
            }

        }else{

            sendMsg(socket, "Utente non registrato\n");

        }

    }

    return email;

}

void playGame(string player1, string player2){

    int solution = rand() % 100;
    cout << "Numero da indovinare: " << solution << endl;
    bool indovinato = false;
    int socket1, socket2;

    {
        lock_guard<mutex> lock(serverMutex);

        socket1 = Users[player1].socket;
        socket2 = Users[player2].socket;

    }

    while (!indovinato){
        
        sendMsg(socket1, "Qual'è il tuo tentativo?\n");
        sendMsg(socket2, "Qual'è il tuo tentativo?\n");

        int guess1 = stoi(recvMsg(socket1));
        cout << "Mossa del giocate " << player1 << " :" << guess1 << endl;
        if (guess1 == solution){
            indovinato = true;
            cout << "Il giocatore " << player1 << " ha indovinato la risposta\n";
            sendMsg(socket1, "Indovinato!\n");
            sendMsg(socket2, "Il giocatore: " + player1 + " ha indovinato\n");

            sendMsg(socket1, "Partita terminata\n");
            sendMsg(socket2, "Partita terminata\n");
            cout << "Partita terminata" << endl;

            {

                lock_guard<mutex> lock(serverMutex);

                Users[player1].inGame = false;
                Users[player2].inGame = false;

            }

            sendMsg(socket1, "Entrato nella lobby. Comandi disponibili --> PLAY\n");
            sendMsg(socket2, "Entrato nella lobby. Comandi disponibili --> PLAY\n");
            break;
        }
        else if(guess1 < solution){

            sendMsg(socket1, "Maggiore\n");

        }
        else if(guess1 > solution){

            sendMsg(socket1, "Minore\n");

        }
        else if (guess1 < 0 || guess1 > 100){

            sendMsg(socket1, "Il numero è compreso tra 0 e 100\n");

        }

        int guess2 = stoi(recvMsg(socket2));
        cout << "Mossa del giocate " << player2 << " :" << guess2 << endl;
        if (guess2 == solution){
            indovinato = true;
            cout << "Il giocatore " << player1 << " ha indovinato la risposta\n";
            sendMsg(socket2, "Indovinato!\n");
            sendMsg(socket1, "Il giocatore: " + player2 + " ha indovinato\n");

            sendMsg(socket2, "Partita terminata\n");
            sendMsg(socket1, "Partita terminata\n");
            cout << "Partita terminata" << endl;

            {

                lock_guard<mutex> lock(serverMutex);

                Users[player1].inGame = false;
                Users[player2].inGame = false;

            }

            sendMsg(socket1, "Entrato nella lobby. Comandi disponibili --> PLAY\n");
            sendMsg(socket2, "Entrato nella lobby. Comandi disponibili --> PLAY\n");
            break;

        }
        else if (guess2 < 0 || guess2 > 100){

            sendMsg(socket2, "Il numero è compreso tra 0 e 100\n");

        }
        else if(guess2 < solution){

            sendMsg(socket2, "Maggiore\n");

        }
        else if(guess2 > solution){

            sendMsg(socket2, "Minore\n");

        }
        
    }

}

void Game(){

    string player1, player2;

    {

        lock_guard<mutex> lock(serverMutex);

        if (waitingList.size() < 2)
            return;

        if (waitingList.size() >= 2){

            player1 = waitingList.front();
            waitingList.pop();

            player2 = waitingList.front();
            waitingList.pop();

            Users[player1].waiting = false;
            Users[player1].inGame = true;
            Users[player2].waiting = false;
            Users[player2].inGame = true;
            
        }
    }

    thread GameThread(playGame, player1, player2);
    GameThread.join();

}

void handleClient(int socket){

    sendMsg(socket, "Comandi disponibili --> REGISTER | LOGIN | PLAY\n");

    string receive, email;

    while (true){

        
        bool occupato = false;

        {
            lock_guard<mutex> lock(serverMutex);

            if (!email.empty() && Users.find(email) != Users.end()) {
                occupato = Users[email].waiting || Users[email].inGame;
            }
        }

        if (occupato) {
            this_thread::sleep_for(chrono::milliseconds(200));
            continue;
        }

        receive = recvMsg(socket);
        if (receive == "REGISTER" || receive == "LOGIN"){
            email = comands(socket, receive);
        }
        else if (receive == "PLAY"){

            bool waiting = false;

            {

                lock_guard<mutex> lock(serverMutex);

                if (!email.empty() && Users[email].loggend && Users.find(email) != Users.end() || !Users[email].waiting && !Users[email].inGame){
                    Users[email].waiting = true;
                    waitingList.push(email);
                }else{
                    waiting = true;
                }
            }

            if (!waiting){
                sendMsg(socket, "Sei in coda per la partita\n");
            }else{
                sendMsg(socket, "Sei già in coda\n");
            }

            Game();
        }
        else if (receive == "QUIT"){

            sendMsg(socket, "Arrivederci!\n");
            cout << "Utente"  << email << " disconnesso\n";

            {

                lock_guard<mutex> lock(serverMutex);

                if (!email.empty()){

                    Users[email].loggend = false;
                    Users[email].waiting = false;
                    close(Users[email].socket);

                }

            }

            break;

        }else{
            sendMsg(socket, "Comando non riconosciuto\n");
            continue;
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
    cout << "Server Game in ascolto...\n";

    while (true){

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore di connessione con il client\n");
            continue;
        }
        cout << "Client connesso" << endl;

        thread t(handleClient, new_sockfd);
        t.detach();

    }

    close(new_sockfd);
    close(server_fd);

    return 0;

}