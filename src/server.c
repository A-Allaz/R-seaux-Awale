#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "game.h"
#include "server.h"
#include "network.h"

int handle(int client_socket, int pid);

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

    // Wait for client to connect
    while (1) {
        addr_len = sizeof (cli_addr);

        // Client connects, attempt to accept
        client_socket = accept(server_socket, (struct sockaddr*) &cli_addr, &addr_len);

        if (client_socket < 0) {
            printf("Error accepting\n");
            exit(errno);
        }

        // Fork into thread to handle asynchronously
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
            handle(client_socket, pid);
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

int handle(int client_socket, const int pid) {
    bool logged_in = false;
    char username[MAX_NAME_LENGTH] = {'\0'};

    while (true) {
        Request req = empty_request();
        if (receive_request(client_socket, &req)) {
            fprintf(stderr, "%d Error: could not get request\n", pid);
            break;
        }

//        print_request(&req);

        switch (req.action) {
            case LOGIN: {
                if (! login(client_socket, req.arguments, username, pid)) {
                    logged_in = true;  // set logged_in flag to true on successful login
                }
                continue;
            }

            case CHALLENGE: {
                challenge(client_socket, req.arguments, pid);
                continue;
            }

            case ACCEPT: {
                accept_request(client_socket, req.arguments, pid);
                continue;
            }

            case LIST: {
                list(client_socket, pid);
                continue;
            }

            case MOVE: {
                move(client_socket, req.arguments, pid);
                continue;
            }
        }
    }

    // Log out at end
    // Handle disconnection cleanup if user is logged in
    if (logged_in) {
        printf("%d User %s disconnected, logging out\n", pid, username);

        GameData gameData;
        if (parse_json(&gameData, "game.json") == 0) {
            // Mark the user as offline
            for (int i = 0; i < gameData.player_count; i++) {
                if (strcmp(gameData.players[i].name, username) == 0) {
                    gameData.players[i].online = false;
                    break;
                }
            }

            // Save updated game data to JSON
            if (save_to_json("game.json", &gameData) != 0) {
                fprintf(stderr, "%d Error: Failed to save updated game data to JSON\n", pid);
            } else {
                printf("%d Successfully logged out user %s\n", pid, username);
            }
        } else {
            fprintf(stderr, "%d Error: Failed to parse game data for logout\n", pid);
        }
    }

    return 0;
}
