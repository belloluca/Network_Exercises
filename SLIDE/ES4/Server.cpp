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
#include "protocol.h"

using namespace std;

mutex serverMutex;

struct User {
    int socket;
    char name[MAX_NAME];
    char password[MAX_PASSWORD];
    bool logged;
    bool busy;
};

map<string, User> Users;

void manageUser(int socket, Message msg) {
    User user;
    Message response;
    strncpy(response.name, "SERVER", sizeof(response.name));

    if (strcmp(msg.comand, "REGISTER") == 0) {
        while (strcmp(msg.name, "") == 0) {
            strncpy(response.message, "Il campo 'name' deve essere compilato", sizeof(response.message));
            sendMsg(socket, response);
            msg = rcvMsg(socket);
        }
        strncpy(user.name, msg.name, sizeof(user.name));
        while (strcmp(msg.password, "") == 0) {
            strncpy(response.message, "Il campo 'password' deve essere compilato", sizeof(response.message));
            sendMsg(socket, response);
            msg = rcvMsg(socket);
        }

        {
            lock_guard<mutex> lock(serverMutex);
            string name = msg.name;
            if (Users.find(name) == Users.end()) {
                strncpy(user.name, msg.name, sizeof(MAX_NAME));
                user.name[sizeof(user.name) - 1] = '\0';
                strncpy(user.password, msg.password, sizeof(user.password));
                user.busy = false;
                user.logged = false;
                user.socket = -1;
                               
                Users[name] = user;
                cout << "Utente " << user.name << " registrato" << endl;
                strncpy(response.message, "Utente registrato con successo", sizeof(response.message));
            } else {
                strncpy(response.message, "Utente già registrato", sizeof(response.message));
            }
        }
        sendMsg(socket, response);

    } else if (strcmp(msg.comand, "LOGIN") == 0) {
        while (strcmp(msg.name, "") == 0) {
            strncpy(response.message, "Il campo 'name' deve essere compilato", sizeof(response.message));
            sendMsg(socket, response);
            msg = rcvMsg(socket);
        }
        strncpy(user.name, msg.name, sizeof(user.name));
        while (strcmp(msg.password, "") == 0) {
            strncpy(response.message, "Il campo 'password' deve essere compilato", sizeof(response.message));
            sendMsg(socket, response);
            msg = rcvMsg(socket);
        }

        string name = msg.name;

        bool logged = false, registered = false, error = false;
        {
            lock_guard<mutex> lock(serverMutex);

            if (Users.find(name) != Users.end()) {
                registered = true;
                if (!Users[name].logged) {
                    if (strcmp(Users[name].password, msg.password) == 0) {
                        Users[name].logged = true;
                        Users[name].busy = false;
                        Users[name].socket = socket;
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
                    strncpy(response.message, "Password errata", sizeof(response.message));
                    sendMsg(socket, response);
                    msg = rcvMsg(socket);
                    while (strcmp(msg.password, "") == 0) {
                        strncpy(response.message, "Il campo 'password' deve essere compilato", sizeof(response.message));
                        sendMsg(socket, response);
                        msg = rcvMsg(socket);
                    }

                    {
                        lock_guard<mutex> lock(serverMutex);

                        if (strcmp(Users[name].password, msg.password) == 0) {
                            error = false;

                            strncpy(response.message, "Utente autenticato con successo", sizeof(response.message));

                            Users[name].logged = true;
                            Users[name].busy = false;
                            Users[name].socket = socket;

                            break;
                        }
                    }
                }
                strncpy(response.message, "Utente autenticato con successo", sizeof(response.message));
            } else {
                strncpy(response.message, "Utente già autenticato", sizeof(response.message));
            }
        } else {
            strncpy(response.message, "Utente non registrato", sizeof(response.message));
        }
        sendMsg(socket, response);
    }

}

void Inoltro(string mittente, int socket1, string dest, int socket2) {
    Message msg;

    while (true) {
        msg = rcvMsg(socket1);

        if (strcmp(msg.message, "EXIT") == 0) {
            cout << "Chat tra " << mittente << " e " << dest << " terminata" << endl;
            {
                lock_guard<mutex> lock(serverMutex);

                Users[mittente].busy = false;
                Users[dest].busy = false;
            }
            strncpy(msg.message, "Uscita dalla chat...", sizeof(msg.message));
            sendMsg(socket1, msg);
            sendMsg(socket2, msg);
            break;
        }
        sendMsg(socket2, msg);
    }

}

void chat(int socket, Message receive) {
    Message msg;

    strncpy(msg.name, "SERVER", sizeof(msg.name));
    strncpy(msg.message, "Con chi vuoi parlare?", sizeof(msg.message));
    sendMsg(socket, msg);

    receive = rcvMsg(socket);

    string mittente = receive.name;
    string dest = receive.message;
    bool exist = false, busy = false, logged = false;
    int peer_socket;

    {
        lock_guard<mutex> lock(serverMutex);

        if (Users.find(dest) != Users.end()) {
            exist = true;
            if (Users[dest].logged) {
                logged = true;
                if (Users[dest].busy) {
                    busy = true;
                } else {
                    peer_socket = Users[dest].socket;
                }
            }
        }
    }

    if (exist) {
        if (logged) {
            if (!busy) {
                cout << "Chat avviata tra " << mittente << " e " << dest << endl; 
                strncpy(msg.message, "Chat avviata", sizeof(msg.message));

                sendMsg(socket, msg);
                sendMsg(peer_socket, msg);

                thread t1(Inoltro, mittente, socket, dest, peer_socket);
                thread t2(Inoltro, dest, peer_socket, mittente, socket);

                {
                    lock_guard<mutex> lock(serverMutex);

                    Users[mittente].busy = true;
                    Users[dest].busy = true;
                }


                t1.detach();
                t2.detach();
            } else {
                strncpy(msg.message, "Utente occupato", sizeof(msg.message));
                sendMsg(socket, msg);
            }
        } else {
            strncpy(msg.message, "Utente offline", sizeof(msg.message));
            sendMsg(socket, msg);
        }
    } else {
        strncpy(msg.message, "Utente inesistente", sizeof(msg.message));
        sendMsg(socket, msg);
    }
    

}

void handleClient(int socket) {

    Message msg;
    Message receive;

    strncpy(msg.name, "SERVER", sizeof(msg.name));
    strncpy(msg.message, "Comandi disponibili: REGISTER | LOGIN | CHAT | QUIT", sizeof(msg.message));
    sendMsg(socket, msg);
    
    while (true) {
            while (!Users[receive.name].busy){
                receive = rcvMsg(socket);

                if ((strcmp(receive.comand, "REGISTER") == 0) || (strcmp(receive.comand, "LOGIN") == 0)) {
                    manageUser(socket, receive);
                } else if (strcmp(receive.comand, "CHAT") == 0) {
                    string name = receive.name;
                    if (name.empty()) {
                        strncpy(msg.message, "Devi prima registrarti", sizeof(msg.message));
                        sendMsg(socket, msg);
                        continue;
                    } else {
                        bool registered = false, logged = false, busy = false;
                        {
                            lock_guard<mutex> lock(serverMutex);

                            if (Users.find(name) != Users.end()) {
                                registered = true;
                                if (Users[name].logged) {
                                    logged = true;
                                    if (Users[name].busy) {
                                        busy = true;
                                    }
                                }
                            }
                        }

                        if (registered) {
                            if (logged) {
                                if (!busy) {
                                    chat(socket, receive);
                                }
                            } else {
                                strncpy(msg.message, "Devi prima autenticarti", sizeof(msg.message));
                                sendMsg(socket, msg);
                            }
                        } else {
                            strncpy(msg.message, "Devi prima registrarti", sizeof(msg.message));
                            sendMsg(socket, msg);
                        }
                    }
                } else if (strcmp(receive.comand, "QUIT") == 0) {
                    string name = receive.name;
                    if (name.empty()) {
                        cout << "Utente disconnesso\n";
                        strncpy(msg.message, "Arrivederci", sizeof(msg.message));
                        sendMsg(socket, msg);
                        close(socket);
                        return;
                    } else {
                        {
                            lock_guard<mutex> lock(serverMutex);
                            if (Users.find(name) != Users.end()) {
                                strncpy(msg.message, "Arrivederci", sizeof(msg.message));
                                sendMsg(socket, msg);

                                Users[name].logged = false;
                                Users[name].busy = false;
                                Users[name].socket = -1;

                                cout << "Utente " << name << " disconnesso" << endl;
                                return;
                            }
                        }
                    }
                } else {
                    strncpy(msg.name, "SERVER", sizeof(msg.name));
                    strncpy(msg.message, "Comando non disponibile", sizeof(msg.message));
                    sendMsg(socket, msg);
                }
            
        }
    }

    close(socket);

}

int main(int argc, char* argv[]) {

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <port>\n";
        return -1;
    }

    int port = stoi(argv[1]);

    int server_fd, new_sockfd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("Errore nella socket\n");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Errore nel bind\n");
        return -1;
    }

    listen(server_fd, 10);
    cout << "Server in ascolto\n";

    while (true) {
        new_sockfd = accept(server_fd, (struct sockaddr*)&client_addr, &len);
        if (new_sockfd < 0) {
            perror("Errore nella connessione\n");
            continue;
        }
        cout << "Client connesso\n";

        thread t(handleClient, new_sockfd);
        t.detach();
    }

    close(server_fd);
    return 0;

}