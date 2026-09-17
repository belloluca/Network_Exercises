#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <mutex>
#include <map>
#include <thread>

using namespace std;

#define BUFFER 1024

mutex serverMutex;

struct User {
    string email;
    string password;
    bool logged;
    bool busy;
    int socket;
};

map<string, User> Users;

bool occupied = false;

void sendMsg(int socket, string msg) {
    msg += "\n";
    if (send(socket, msg.c_str(), msg.size(), 0) < 0) {
        perror("Errore nell'invio dei dati\n");
    }
}

string rcvMsg(int socket) {
    char buffer[BUFFER];

    int n = recv(socket, buffer, BUFFER - 1, 0);
    if (n <= 0) {
        perror("Errore nella ricezione dei dati\n");
    }
    buffer[n] = '\0';

    return string(buffer);
}


string manageUser(int socket, string comand) {
    string email, password;

    if (comand == "REGISTER") {
        bool registered = false;
        User user;

        sendMsg(socket, "Inserisci email: ");
        email = rcvMsg(socket);

        while (email.empty()) {
            sendMsg(socket, "Email incoretta");
            sendMsg(socket, "Inserisci email: ");
            email = rcvMsg(socket);
        }

        sendMsg(socket, "Inserisci password: ");
        password = rcvMsg(socket);

        while (password.empty()) {
            sendMsg(socket, "password incoretta");
            sendMsg(socket, "Inserisci password: ");
            password = rcvMsg(socket);
        }

        {
            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) == Users.end()) {
                user.email = email;
                user.password = password;
                user.logged = false;
                user.busy = false;
                user.socket = -1;
                                
                Users[email] = user;
                cout << "Utente " << email <<  " registrato\n";
            } else {
                registered = true;
            }
        }

        if (!registered) {
            sendMsg(socket, "Utente registrato con successo");
            sendMsg(socket, "Comandi disponibili: LOGIN | TEXT | QUIT");
        } else {
            sendMsg(socket, "Utente già registrato");
        }

    }
    else if (comand == "LOGIN") {
        bool logged = false, error = false, registered = false;

        sendMsg(socket, "Inserisci email: ");
        email = rcvMsg(socket);

        while (email.empty()) {
            sendMsg(socket, "Email incoretta");
            sendMsg(socket, "Inserisci email: ");
            email = rcvMsg(socket);
        }

        sendMsg(socket, "Inserisci password: ");
        password = rcvMsg(socket);

        while (password.empty()) {
            sendMsg(socket, "password incoretta");
            sendMsg(socket, "Inserisci password:");
            password = rcvMsg(socket);
        }

        {
            lock_guard<mutex> lock(serverMutex);

            if (Users.find(email) != Users.end()) {
                registered = true;
                if (!Users[email].logged) {
                    if (Users[email].password == password) {
                        Users[email].logged = true;
                        Users[email].busy = false;
                        Users[email].socket = socket;

                        cout << "Utente " << Users[email].email << " loggato\n";
                    } else {
                        error = true;
                    }
                } else {
                    logged = true;
                }
            }
        }

        if (registered) {
            if (!logged) {
                while (error) {
                    sendMsg(socket, "Password errata\n");
                    sendMsg(socket, "Inserisci password: ");
                    password = rcvMsg(socket);

                    while (password.empty()) {
                        sendMsg(socket, "password incoretta");
                        sendMsg(socket, "Inserisci password:");
                        password = rcvMsg(socket);
                    }

                    {
                        lock_guard<mutex> lock(serverMutex);

                        if (Users[email].password == password) {
                            error = false;
                            Users[email].logged = true;
                            Users[email].busy = false;
                            Users[email].socket = socket;

                            cout << "Utente " << Users[email].email << " loggato\n"; 
                        }
                    }
                }
                sendMsg(socket, "Utente autenticato\n");
                sendMsg(socket, "Comandi disponibili: TEXT | QUIT");
            } else {
                sendMsg(socket, "Utente già loggato\n");
            }
        } else {
            sendMsg(socket, "Utente non registrato\n");
        }
    }

    return email;

}

void Inoltro(int socket1, int socket2, string mittente, string dest) {
    string msg, receive;

    while (true) {

        receive = rcvMsg(socket1);
        msg = mittente + ": " + receive;
        sendMsg(socket2, msg);

        if (receive == "QUIT") {
            sendMsg(socket2, "Utente disconnesso");
            cout << "Utente " << mittente << " disconnesso";

            {
                lock_guard<mutex> lock(serverMutex);

                Users[mittente].busy = false;
                Users[dest].busy = false;
            }
            occupied = false;
            break;
        }
    }

}

void chat(int socket1, string mittente) {

    string dest;
    int socket2;

    sendMsg(socket1, "Con chi vuoi parlare?");
    dest = rcvMsg(socket1);

    while (dest.empty()) {
        sendMsg(socket1, "Inserisci una email valida");
        sendMsg(socket1, "Con chi vuoi parlare?");
        dest = rcvMsg(socket1);
    }

    bool exist = false;

    {
        lock_guard<mutex> lock(serverMutex);

        if (Users.find(dest) != Users.end() && Users[dest].logged && !Users[dest].busy) {
            exist = true;

            socket2 = Users[dest].socket;

            Users[mittente].busy = true;
            Users[dest].busy = true;

            occupied = true;

            cout << "Chat avviata tra " << mittente << " e " << dest << endl;
        }
    }

    if (exist) {
        sendMsg(socket1, "Chat avviata con " + mittente);
        sendMsg(socket2, "Chat avviata con " + dest);

        thread t1(Inoltro, socket1, socket2, mittente, dest);
        thread t2(Inoltro, socket2, socket1, dest, mittente);

        t1.detach();
        t2.detach();
    } else {
        sendMsg(socket1, "Utente inesistente");
    }

}

void handleClient(int socket) {

    string comand, email;

    sendMsg(socket, "Comandi disponibili: REGISTER | LOGIN | TEXT | QUIT");

    while (true) {
        while (!Users[email].busy){
            comand = rcvMsg(socket);

            if (comand == "LOGIN" || comand == "REGISTER") {
                email = manageUser(socket, comand);
            }
            else if (comand == "TEXT") {
                bool registered = false, logged = false, busy = false;
                if (!email.empty()) {
                    {
                        lock_guard<mutex> lock(serverMutex);

                        if (Users.find(email) != Users.end()) {
                            registered = true;
                            if (Users[email].logged) {
                                logged = true;
                                if (Users[email].busy) {
                                    busy = true;
                                }
                            }
                        }
                    }

                    if (registered) {
                        if (logged) {
                            if (busy) {
                                sendMsg(socket, "Utente già in una chat");
                            } else {
                                chat(socket, email);
                            }
                        } else {
                            sendMsg(socket, "Utente non autenticato");
                        }
                    } else {
                        sendMsg(socket, "Utente non registrato");
                    }
                } else {
                    sendMsg(socket, "Email non valida");
                }
            }
            else if (comand == "QUIT") {
                bool exist = false;
                {
                    lock_guard<mutex> lock(serverMutex);

                    if (!email.empty()) {
                        if (Users.find(email) != Users.end()) {
                            cout << "Utente " << email << " disconnesso\n";

                            Users[email].busy = false;
                            Users[email].logged = false;
                            Users[email].socket = -1;
                        }
                    } else {
                        cout << "Utente disconnesso\n";
                    }
                }

                sendMsg(socket, "Arrivederci");
                occupied = false;
                break;
            }
            else {
                sendMsg(socket, "Comando non disponibile");
            }

    }
    }

}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>\n";
        return -1;
    }

    int port = stoi(argv[1]);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 10);
    cout << "Server in ascolto\n";

    int new_sockfd;
    while (true) {
        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0) {
            perror("Errore nella connessione con il client\n");
            continue;
        }
        cout << "Client connesso\n";

        thread t(handleClient, new_sockfd);
        t.detach();
    }

    close(server_fd);
    return 0;
}