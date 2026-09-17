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
#include <fstream>

using namespace std;

#define BUFFER 1024

mutex serverMutex;

struct User {
    string email;
    string password;
    bool logged;
    bool busy;
    int socket;

    string address;
    int port;
};

map<string, User> Users;

bool occupied = false;

void saveUser(User user) {

    ofstream file("users.txt", ios::app);

    if (!file.is_open()){
        cout << "Eerrore nell'apertura del file\n";
        return;
    }

    file << user.email << " " << user.password << endl;

    file.close();
}

void loadFile() {

    ifstream file("users.txt");

    if (!file.is_open()){
        cout << "Errore nell'apertura del file\n";
    }

    string email, password;
    User user;

    while (file >> email >> password) {
        user.email = email;
        user.password = password;
        user.busy = false;
        user.logged = false;
        user.address = "";
        user.port = 0;
        user.socket = -1;

        Users[email] = user;
    }

    file.close();

    cout << "Utenti caricati\n";

}

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


string manageUser(int socket, string comand, string ip) {
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
                user.address = ip;
                                
                Users[email] = user;
                saveUser(user);

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

        sendMsg(socket, "Inserisci porta P2P: ");
        string port = rcvMsg(socket);

        while (port.empty()) {
            sendMsg(socket, "Porta non valida");
            sendMsg(socket, "Inserisci porta P2P:");
            port = rcvMsg(socket);
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
                        Users[email].port = stoi(port);

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

    string ip1, ip2;
    int port1, port2;

    {
        lock_guard<mutex> lock(serverMutex);

        if (Users.find(dest) != Users.end() && Users[dest].logged && !Users[dest].busy) {
            exist = true;

            socket2 = Users[dest].socket;

            Users[mittente].busy = true;
            Users[dest].busy = true;

            ip1 = Users[mittente].address;
            port1 = Users[mittente].port;

            ip2 = Users[dest].address;
            port2 = Users[dest].port;

            cout << "Chat avviata tra " << mittente << " e " << dest << endl;
        }
    }

    if (exist) {
        sendMsg(socket2, "Info di " + mittente + ": " + ip1 + " | " + to_string(port1));
        sendMsg(socket1, "Info di " + dest + ": " + ip2 + " | " + to_string(port2));
    } else {
        sendMsg(socket1, "Utente inesistente");
    }

}

void handleClient(int socket, string client_ip) {

    string comand, email;

    sendMsg(socket, "Comandi disponibili: REGISTER | LOGIN | TEXT | QUIT");

    while (true) {
        while (!Users[email].busy){
            comand = rcvMsg(socket);

            if (comand == "LOGIN" || comand == "REGISTER") {
                email = manageUser(socket, comand, client_ip);
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

    loadFile();

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

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, INET_ADDRSTRLEN);

        thread t(handleClient, new_sockfd, ip);
        t.detach();
    }

    close(server_fd);
    return 0;
}