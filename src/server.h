//
// Defines functions for the client
//

#ifndef AWALEGAME_SERVER_H
#define AWALEGAME_SERVER_H

#include "utils.h"

// Create new username if it doesn't already exist and mark as active. Return true for success, false for error
int login(int socket, const char args[3][255], const int pid) {
    printf("%d LOGIN for socket: %d\n", pid, socket);

    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", pid);
        send(socket, "false", 5, 0);
        return -1;
    }

    // Check if the username exists in the player list
    bool userExists = false;
    for (int i = 0; i < gameData.player_count; i++) {
        if (strcmp(gameData.players[i].name, args[0]) == 0) {
            // Username exists, set the player as online
            gameData.players[i].online = true;
            userExists = true;
            break;
        }
    }

    // If the user does not exist, add them to the list
    if (!userExists) {
        if (gameData.player_count < MAX_PLAYERS) {
            strcpy(gameData.players[gameData.player_count].name, args[0]);
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
    printf("%d CHALLENGE for socket: %d\n", pid, socket);
    return 0;
}

// TODO
int accept_request(int socket, char args[3][255], const int pid) {
    printf("%d ACCEPT for socket: %d\n", pid, socket);
    return 0;
}

// Return all currently online users as json
int list(int socket, const int pid) {
    printf("%d LIST for socket: %d\n", pid, socket);

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
    printf("%d MOVE for socket: %d\n", pid, socket);

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

#endif //AWALEGAME_SERVER_H
