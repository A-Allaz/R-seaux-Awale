#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "game.h"
#include "network.h"
#include "utils.h"

#define PORT_NO 3000

int handle(int client_socket, int pid);

int list(int socket, int pid);

int login(int socket, const char args[3][255], int pid);

int challenge(int socket, char args[3][255], int pid);

int move(int socket, char args[3][255], int pid);

int accept_request(int socket, char args[3][255], int pid);

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
    bzero((char*)&serv_addr, sizeof(serv_addr));
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

int handle(int client_socket, const int pid) {
    Request req;
    if (receive_request(client_socket, &req)) {
        fprintf(stderr, "%d Error: could not get request", pid);
        return 1;
    }

    print_request(&req);

    switch (req.action) {
        case LOGIN: {
            if (login(client_socket, req.arguments, pid)) {
                return 1;
            } break;
        }

        case CHALLENGE: {
            if (challenge(client_socket, req.arguments, pid)) {
                return 1;
            } break;
        }

        case ACCEPT:
            if (accept_request(client_socket, req.arguments, pid)) {
                return 1;
            } break;
        case LIST:
            if (list(client_socket, pid)) {
                return 1;
            } break;
        case MOVE:
            if (move(client_socket, req.arguments, pid)) {
                return 1;
            } break;
    }

    return 0;
}

// Create new username if it doesn't already exist and mark as active. Return true for success, false for error
int login(int socket, const char args[3][255], const int pid) {
    printf("%d LOGIN for socket: %d", pid, socket);

    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", pid);
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
            fprintf(stderr, "%d Error: Player list is full\n", pid);
            send(socket, "false", 5, 0);
            return -1;
        }
    }

    // Save the updated GameData back to the JSON file
    if (save_to_json("game.json", &gameData)) {
        fprintf(stderr, "%d Error: Failed to save updated game data to JSON\n", pid);
        send(socket, "false", 5, 0);
        return -1;
    }

    send(socket, "true", 4, 0);
    return 0;
}

// TODO
int challenge(int socket, char args[3][255], const int pid) {
    printf("%d CHALLENGE for socket: %d", pid, socket);
    return 0;
}

// TODO
int accept_request(int socket, char args[3][255], const int pid) {
    printf("%d ACCEPT for socket: %d", pid, socket);
    return 0;
}

// Return all currently online users as json
int list(int socket, const int pid) {
    printf("%d LIST for socket: %d", pid, socket);

    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", pid);
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
        fprintf(stderr, "%d Error: Failed to serialize JSON\n", pid);
        return -1;
    }

    // Send the JSON string to the client
    size_t jsonStringLength = strlen(jsonString);
    if (send(socket, jsonString, jsonStringLength, 0) == -1) {
        fprintf(stderr, "%d Error: Failed to send online players list\n", pid);
        free(jsonString);
        return -1;
    }

    free(jsonString); // Free the serialized JSON string
    return 0;
}

// Move your pieces given args: [user1, user2, move]. Return new game state as json
int move(int socket, char args[3][255], const int pid) {
    printf("%d MOVE for socket: %d", pid, socket);

    // Get game data
    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", pid);
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
        fprintf(stderr, "%d Error: Failed to save updated game data to JSON\n", pid);
        return -1;
    }

    // Broadcast the updated game state to all players
    char* json_string = game_to_json_string(&gameData.games[index]);

    // Send the JSON string to the client
    if (send(socket, json_string, strlen(json_string), 0) == -1) {
        fprintf(stderr, "%d Error: Failed to send game state\n", pid);
        free(json_string);  // Free the string created by cJSON_Print
        return -1;
    }

    // Free the JSON objects after use
    free(json_string);  // Free the string created by cJSON_Print
    return 0;
}
