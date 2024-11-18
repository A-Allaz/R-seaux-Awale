//
// Created by Christopher Wilson on 08/11/2024.
//


#include "game.h"
#include "network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

int handle(int newsockfd);

int list(int newsockfd);

int make_move(Game *game, int slot);

int login(int newsockfd, const char args[3][255]);

int main() {
    const in_port_t PORT_NO = 3000;
    int server_socket, client_socket;
    socklen_t clilen;

    struct sockaddr_in cli_addr,serv_addr;

    printf ("Server starting...\n");

    // Open socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf ("Could not open socket\n");
        exit(0);
    }

    // Initialise parameters
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family       = AF_INET;
    serv_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    serv_addr.sin_port         = PORT_NO;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error binding\n");
        close(server_socket);
        exit(errno);
    }

    // Begin listening
    if (listen(server_socket, 5) < 0) {
        printf("Error on listen\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT_NO);

    // Wait for client to connect
    while (1) {
        clilen = sizeof (cli_addr);

        // Client connects, attempt to accept
        client_socket = accept(server_socket, (struct sockaddr*) &cli_addr, &clilen);

        if (client_socket < 0) {
            printf("Error accepting\n");
            exit(errno);
        }

        printf("Connection accepted\n");

        // Fork into thread to handle asynchronously
        int pid = fork();

        // Error case
        if (pid < 0) {
            perror("Error on fork");
            close(client_socket);
            continue;
        }

        // Child thread
        else if (pid == 0) {
            close(server_socket);
            handle(client_socket);
            close(client_socket);
            exit(0); /* Terminate child process */
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

int handle(int newsockfd) {
    Request req;
    if (receive_request(newsockfd, &req)) {
        perror("error getting request");
        return 1;
    }

    switch (req.action) {
        case LOGIN:
            if (login(newsockfd, req.arguments)) {
                return 1;
            }
        case CHALLENGE:
            break;
        case ACCEPT:
            break;
        case LIST:
            if (list(newsockfd)) {
                return 1;
            }
        case MOVE:
            break;
    }

    return 0;
}

int login(int newsockfd, const char args[3][255]) {
    char username[255];
    strcpy(args[0], username);

    printf("%s", username);

    return 0;
}

int list(int newsockfd) {
    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        return -1;
    }

    // Create a JSON array to hold online players
    cJSON *onlinePlayers = cJSON_CreateArray();

    for (int i = 0; i < gameData.player_count; i++) {
        if (gameData.players[i].online) {
            // Add the online player's name to the JSON array
            cJSON_AddItemToArray(onlinePlayers, cJSON_CreateString(gameData.players[i].name));
        }
    }

    // Serialize the JSON array to a string
    char *jsonString = cJSON_PrintUnformatted(onlinePlayers);
    cJSON_Delete(onlinePlayers); // Free JSON object memory

    if (!jsonString) {
        fprintf(stderr, "Error: Failed to serialize JSON\n");
        return -1;
    }

    // Send the JSON string to the client
    size_t jsonStringLength = strlen(jsonString);
    if (send(newsockfd, jsonString, jsonStringLength, 0) == -1) {
        perror("Error sending online players list");
        free(jsonString);
        return -1;
    }

    free(jsonString); // Free the serialized JSON string
    return 0;
}

// Perform move given a slot (ASSUMED TO BE VALID)
int make_move(Game *game, int slot) {
    // Perform turn
    play_turn(game, slot - 1);

    // Save game state
    save_game_state("game.json", game);

    // Broadcast game to players
    broadcast_game_state();
}
