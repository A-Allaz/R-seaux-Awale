#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
//#include <pthread.h>

#include "game.h"
#include "server.h"
#include "network.h"

//pthread_mutex_t socket_details_mutex = PTHREAD_MUTEX_INITIALIZER;
//
//SocketDetail socket_details[MAX_SOCKETS];

int handle(int client_socket);

int main() {
    int server_socket, client_socket;
    socklen_t addr_len;

    struct sockaddr_in cli_addr, serv_addr;

    printf("Server starting...\n");

    // Open socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Could not open socket");
        exit(EXIT_FAILURE);
    }

    // Initialise parameters
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT_NO);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error binding");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Begin listening
    if (listen(server_socket, 5) < 0) {
        perror("Error on listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n\n", PORT_NO);

    // Initialise the socket_details list before sockets can connect
//    initialize_socket_details(socket_details, &socket_details_mutex);

    // Wait for client to connect
    while (1) {
        addr_len = sizeof (cli_addr);

        // Client connects, attempt to accept
        client_socket = accept(server_socket, (struct sockaddr*) &cli_addr, &addr_len);

        if (client_socket < 0) {
            printf("Error accepting\n");
            exit(errno);
        }

//        int* new_socket = malloc(sizeof(int));
//        *new_socket = client_socket;

//        pthread_t thread_id;
//        if (pthread_create(&thread_id, NULL, handle, (void *) new_socket) != 0) {
//            perror("Thread creation failed");
//            close(client_socket);
//            free(new_socket);
//        }
//        pthread_detach(thread_id);

        // Fork into thread to handle asynchronously
//        SocketDetail** details = (SocketDetail **) &socket_details;
        int pid = fork();

        // Error case
        if (pid < 0) {
            perror("Connection accepted, forking unsuccessful\n");
            perror("Error on fork");
            close(client_socket);
            continue;
        }

        // Child thread
        else if (pid == 0) {
            printf("Connection accepted, fork %d\n", pid);
            close(server_socket);
//            handle(client_socket, *details);
            handle(client_socket);
            close(client_socket);
            exit(0);  // Terminate child process
        }

        // Main thread
        else {
            close(client_socket);
        }

    }

    // Close server socket at end
    close(server_socket);

    return 0;
}

int handle(int client_socket) {
    bool logged_in = false;
    char username[MAX_NAME_LENGTH] = {'\0'};

    while (true) {
        Request req = empty_request();
        if (receive_request(client_socket, &req)) {
            fprintf(stderr, "%d Error: could not get request\n", client_socket);
            break;
        }

//        print_request(&req);

        switch (req.action) {
            case LOGIN: {
                if (! login(client_socket, req.arguments, username)) {
                    logged_in = true;  // set logged_in flag to true on successful login
//                    if (add_socket_detail(socket_details, client_socket, username, &socket_details_mutex)) {
//                        fprintf(stderr, "%d Error: max sockets reached\n", client_socket);
//                    }
                }
                continue;
            }

            case LIST: {
                list(client_socket, req.arguments);
                continue;
            }

            case CHALLENGE: {
                challenge(client_socket, req.arguments);
                continue;
            }

            case ACCEPT: {
                accept_request(client_socket, req.arguments);
                continue;
            }

            case DECLINE: {
                decline(client_socket, req.arguments);
                continue;
            }


            case GAME: {
                get_game(client_socket, req.arguments);
                continue;
            }

            case LIST_GAMES: {
                get_all_games(client_socket, req.arguments);
                continue;
            }

            case MOVE: {
//                move(client_socket, req.arguments, socket_details, &socket_details_mutex);
                move(client_socket, req.arguments);
                continue;
            }
        }
    }

    // Log out at end
    // Handle disconnection cleanup if user is logged in
    if (logged_in) {
        printf("%d User %s disconnected, logging out\n", client_socket, username);

//        // Mark as logged out to free up space for new sockets
//        logout_socket_detail(socket_details, client_socket, &socket_details_mutex);

        GameData gameData;
        if (parse_json(&gameData, JSON_FILENAME) == 0) {
            // Mark the user as offline
            for (int i = 0; i < gameData.player_count; i++) {
                if (strcmp(gameData.players[i].name, username) == 0) {
                    gameData.players[i].online = false;
                    break;
                }
            }

            // Save updated game data to JSON
            if (save_to_json(JSON_FILENAME, &gameData) != 0) {
                fprintf(stderr, "%d Error: Failed to save updated game data to JSON\n", client_socket);
            } else {
                printf("%d Successfully logged out user %s\n", client_socket, username);
            }
        } else {
            fprintf(stderr, "%d Error: Failed to parse game data for logout\n", client_socket);
        }
    }

    return 0;
}
