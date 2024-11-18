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

int handle(int client_socket);

int list(int socket);

int login(int socket, const char args[3][255]);

int challenge(int socket, char args[3][255]);

int move(int socket, char args[3][255]);

int accept_request(int socket, char args[3][255]);

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

int handle(int client_socket) {
    Request req;
    if (receive_request(client_socket, &req)) {
        perror("error getting request");
        return 1;
    }

    switch (req.action) {
        case LOGIN:
            if (login(client_socket, req.arguments)) {
                return 1;
            }
        case CHALLENGE:
            if (challenge(client_socket, req.arguments)) {
                return 1;
            }
        case ACCEPT:
            if (accept_request(client_socket, req.arguments)) {
                return 1;
            }
        case LIST:
            if (list(client_socket)) {
                return 1;
            }
        case MOVE:
            if (move(client_socket, req.arguments)) {
                return 1;
            };
    }

    return 0;
}

// Create new username if it doesn't already exist and mark as active. Return true for success, false for error
int login(int socket, const char args[3][255]) {
    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        send(socket, "false", 5, 0);
        return -1;
    }

    // Check if the username exists in the player list
    bool userExists = false;
    for (int i = 0; i < gameData.player_count; i++) {
        if (strcmp(gameData.players[i].name, args[1]) == 0) {
            // Username exists, set the player as online
            gameData.players[i].online = true;
            userExists = true;
            break;
        }
    }

    // If the user does not exist, add them to the list
    if (!userExists) {
        if (gameData.player_count < MAX_PLAYERS) {
            strcpy(gameData.players[gameData.player_count].name, args[1]);
            gameData.players[gameData.player_count].online = true;
            gameData.player_count++; // Increment player count
        } else {
            fprintf(stderr, "Error: Player list is full\n");
            send(socket, "false", 5, 0);
            return -1;
        }
    }

    // Save the updated GameData back to the JSON file
    if (save_to_json("game.json", &gameData)) {
        fprintf(stderr, "Error: Failed to save updated game data to JSON\n");
        send(socket, "false", 5, 0);
        return -1;
    }

    send(socket, "true", 4, 0);
    return 0;
}

// TODO
int challenge(int socket, char args[3][255]) {
    return 0;
}

// TODO
int accept_request(int socket, char args[3][255]) {
    return 0;
}

// Return all currently online users as json
int list(int socket) {
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
    if (send(socket, jsonString, jsonStringLength, 0) == -1) {
        perror("Error sending online players list");
        free(jsonString);
        return -1;
    }

    free(jsonString); // Free the serialized JSON string
    return 0;
}

// Move your pieces given args: [user1, user2, move]. Return new game state as json
int move(int socket, char args[3][255]) {
    // Get game data
    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        send(socket, "false", 5, 0);
        return -1;
    }

    // Find game
    int index = load_game(args[0], args[1]);
    if (index < 0) {
        send(socket, "false", 5, 0);
        return -1;
    }

    int slot = convert_and_validate(args[2]);

    if (slot < 0) {
        send(socket, "false", 5, 0);
        return -1;
    }

    // Perform the player's turn
    play_turn(&gameData.games[index], slot - 1);

    // Save the updated game data back to the JSON file
    if (save_to_json(JSON_FILENAME, &gameData) != 0) {
        fprintf(stderr, "Error: Failed to save updated game data to JSON\n");
        return -1;
    }

    // Broadcast the updated game state to all players
    char* json_string = game_to_json_string(&gameData.games[index]);

    // Send the JSON string to the client
    if (send(socket, json_string, strlen(json_string), 0) == -1) {
        perror("Error sending game state");
        free(json_string);  // Free the string created by cJSON_Print
        return -1;
    }

    // Free the JSON objects after use
    free(json_string);  // Free the string created by cJSON_Print
    return 0;
}
