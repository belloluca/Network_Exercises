#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <thread>
#include <mutex>
#include <map>
#include <queue>

#define PORT 8080
#define BUFFER 1024

using namespace std;

struct Player{

    string email;
    string password;

};

struct Session{

    string email;
    bool logged = false;
    bool waiting = false;
    bool inGame = false;
    int socket = -1;

};

map<string, Player> players;
queue<Session *> waitingLine;
mutex ServerMutex;

bool isValidMove(const string move){

    return move == "sasso" || move == "carta" || move == "forbici";

}

int winnerRoud(const string move1, const string move2){

    if (move1 == move2){
        return 0;
    }

    if ((move1 == "carta" && move2 == "sasso") || (move1 == "sasso" && move2 == "forbici") || (move1 == "forbici" && move2 == "carta")){
        return 1;
    }

    return 2;

}

void playGame(Session *player1, Session *player2){

    string msg;
    int n;

    {

        lock_guard<mutex> lock(ServerMutex);

        player1->inGame = true;
        player2->inGame = true;
        player1->waiting = false;
        player2->waiting = false;

    }

    msg = "Partita trovata";
    
    if ((send(player1->socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio\n");
    }
    if ((send(player2->socket, msg.c_str(), msg.size(), 0)) < 0){
        perror("Errore nell'invio\n");
    }

    char buffer[BUFFER];
    int score1 = 0, score2 = 0;

    for (int round = 1; round <= 3; round++){

        string move1, move2;
        char buffer1[BUFFER], buffer2[BUFFER];

        msg = "ROUND" + to_string(round) + "Inserisci una mossa: carta | forbici | sasso\n";

        if ((send(player1->socket, msg.c_str(), msg.size(), 0)) < 0){
            perror("Errore nell'invio\n");
        }

        if ((send(player2->socket, msg.c_str(), msg.size(), 0)) < 0){
            perror("Errore nell'invio\n");
        }

        n = recv(player1->socket, buffer1, BUFFER, 0);
        if (n <= 0){
            perror("Errore nella ricezione\n");
        }
        buffer1[n] = '\0';
        move1 = string(buffer1);

        n = recv(player2->socket, buffer2, BUFFER, 0);
        if (n <= 0){
            perror("Errore nella ricezione\n");
        }
        buffer2[n] = '\0';
        move2 = string(buffer2);

        while (!isValidMove(move1)){

            msg = "Mossa non compresa\n";
            if ((send(player1->socket, msg.c_str(), msg.size(), 0)) < 0){
                perror("Errore nell'invio\n");
            }

            n = recv(player1->socket, buffer1, BUFFER, 0);
            if (n <= 0){
                perror("Errore nella ricezione\n");
            }
            buffer1[n] = '\0';
            move1 = string(buffer1);

        }

        while (!isValidMove(move2)){

            msg = "Mossa non compresa\n";
            if ((send(player2->socket, msg.c_str(), msg.size(), 0)) < 0){
                perror("Errore nell'invio\n");
            }

            n = recv(player2->socket, buffer2, BUFFER, 0);
            if (n <= 0){
                perror("Errore nella ricezione\n");
            }
            buffer2[n] = '\0';
            move2 = string(buffer2);

        }

        int winner = winnerRoud(move1, move2);

        if (winner == 1){
            score1++;
        }
        else if (winner == 2){
            score2++;
        }

        string result = "RISULTATO DEL ROUND " + player1->email + "=" + move1 + " " + player2->email + "=" + move2 + " SCORE " + to_string(score1) + "-" + to_string(score2);

        if ((send(player1->socket, result.c_str(), result.size(), 0)) < 0){
            perror("Errore nell'invio\n");
        }

        if ((send(player2->socket, result.c_str(), result.size(), 0)) < 0){
            perror("Errore nell'invio\n");
        }

    }

    string finalMsg;

    if (score1 > score2){
        finalMsg = "Partita vinta da: " + player1->email;

    }
    else if (score1 < score2){
        finalMsg = "Partita vinta da: " + player2->email;
    } 
    else{
        finalMsg = "Partita finita in pareggio";
    }

    if ((send(player1->socket, finalMsg.c_str(), finalMsg.size(), 0)) < 0){
        perror("Errore nell'invio\n");
    }

    if ((send(player2->socket, finalMsg.c_str(), finalMsg.size(), 0)) < 0){
        perror("Errore nell'invio\n");
    }

    {

        lock_guard<mutex> lock(ServerMutex);

        player1->inGame = false;
        player2->inGame = false;

    }


}

void findMatch(){

    Session *player1 = nullptr;
    Session *player2 = nullptr;

    {

        lock_guard<mutex> lock(ServerMutex);

        while (!waitingLine.empty() && (!waitingLine.front()->logged || waitingLine.front()->inGame)){
            waitingLine.pop();
        }

        if (waitingLine.size() >= 2){

            player1 = waitingLine.front();
            waitingLine.pop();

            player2 = waitingLine.front();
            waitingLine.pop();

        }

    }

    if (player1 != nullptr && player2 != nullptr){
        thread GameThread(playGame, player1, player2);
        GameThread.detach();
    }

}

void handleClient(int socket){

    string msg, email, password;
    char buffer[BUFFER];
    int n;
    Player giocatore;
    Session session;

    msg = "Inserisci un comando: REGISTER | LOGIN | PLAY\n";

    while (true){  

        if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
            perror("Errore nell'invio\n");
            return;
        }

        n = recv(socket, buffer, BUFFER, 0);
        if (n <= 0){
            perror("Errore nella ricezione dei dati\n");
            return;
        }
        buffer[n] = '\0';

        if (string(buffer) == "REGISTER"){

            msg = "Inserisci email:\n";

            if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                perror("Errore nell'invio\n");
                return;
            }

            n = recv(socket, buffer, BUFFER, 0);
            if (n <= 0){
                perror("Errore nella ricezione dei dati\n");
                return;
            }
            buffer[n] = '\0';

            email = string(buffer);

            msg = "Inserisci la password:\n";

            if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                perror("Errore nell'invio\n");
                return;
            }

            n = recv(socket, buffer, BUFFER, 0);
            if (n <= 0){
                perror("Errore nella ricezione dei dati\n");
                return;
            }
            buffer[n] = '\0';

            password = string(buffer);

            {

                lock_guard<mutex> lock(ServerMutex);

                if (players.find(email) != players.end()){
                    msg = "Giocatore già registrato\n";
                    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                        perror("Errore nell'invio\n");
                        return;
                    }
                }else{

                    msg = "Giocatore registrato\n";
                    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                        perror("Errore nell'invio\n");
                        return;
                    }

                    giocatore.email = email;
                    giocatore.password = password;

                    players[email] = giocatore;

                }

            }

        }
        else if(string(buffer) == "LOGIN"){

            msg = "Inserisci email:\n";

            if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                perror("Errore nell'invio\n");
                return;
            }

            n = recv(socket, buffer, BUFFER, 0);
            if (n <= 0){
                perror("Errore nella ricezione dei dati\n");
                return;
            }
            buffer[n] = '\0';

            email = string(buffer);

            msg = "Inserisci la password:\n";

            if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                perror("Errore nell'invio\n");
                return;
            }

            n = recv(socket, buffer, BUFFER, 0);
            if (n <= 0){
                perror("Errore nella ricezione dei dati\n");
                return;
            }
            buffer[n] = '\0';

            password = string(buffer);

            {

                lock_guard<mutex> lock(ServerMutex);

                if (players.find(email) != players.end()){

                    if (players[email].password == password){

                        msg = "Login effettuato\n";
                        if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                            perror("Errore nell'invio\n");
                            return;
                        }

                        cout << "Login effettuato dall'utente " << email << endl;

                        session.email = email;
                        session.socket = socket;
                        session.logged = true;

                    }else{

                        msg = "Password errata\n";
                        if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                            perror("Errore nell'invio\n");
                            return;
                        }

                    }

                }else{

                    msg = "Email non registrata\n";
                    if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                        perror("Errore nell'invio\n");
                        return;
                    }

                }

            }

        }
        else if (string(buffer) == "PLAY"){

            if (!session.logged){

                msg = "Devi prima autenticarti\n";
                if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                    perror("Errore nell'invio\n");
                    return;
                }

            }
            else if ((session.inGame) || (session.waiting)){

                msg = "Sei già in attesa o in partita\n";
                if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                    perror("Errore nell'invio\n");
                    return;
                }
            }
            else{

                {

                    lock_guard<mutex> lock(ServerMutex);

                    session.waiting = true;

                    waitingLine.push(&session);

                }

                findMatch();

            }

        }
        else if (string(buffer) == "QUIT"){
            
            msg = "Arrivederci\n";
                if ((send(socket, msg.c_str(), msg.size(), 0)) < 0){
                    perror("Errore nell'invio\n");
                    return;
                }

                break;

        }

    }

}

int main(){

    int server_fd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0){
        perror("Errore nella socket\n");
        return -1;
    }

    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if ((bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr))) < 0){
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 10);
    cout << "Game Server in ascolto...\n";

    while (true){

        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0){
            perror("Errore nella connessione con il client\n");
            continue;
        }

        thread t(handleClient, new_sockfd);
        t.detach();

    }

    close(server_fd);

    return 0;

}