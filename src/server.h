//
// Defines functions for the client
//

#ifndef AWALEGAME_SERVER_H
#define AWALEGAME_SERVER_H

#include "utils.h"
#include "network.h"

//#define MAX_SOCKETS 15
//
//typedef struct {
//    int client_socket;
//    bool logged_in;
//    char username[MAX_NAME_LENGTH];
//} SocketDetail;
//
//void initialize_socket_details(SocketDetail* socket_details, pthread_mutex_t* mutex) {
//    pthread_mutex_lock(mutex);
//    for (int i = 0; i < MAX_SOCKETS; i++) {
//        socket_details[i].client_socket = -1;
//        socket_details[i].logged_in = false;
//        socket_details[i].username[0] = '\0'; // Empty string
//    }
//    pthread_mutex_unlock(mutex);
//}
//
//// Connect a username to a socket
//int add_socket_detail(SocketDetail* socket_details, int client_socket, const char *username, pthread_mutex_t* mutex) {
//    printf("Adding socket details with username: %s\n", username);
//    pthread_mutex_lock(mutex);
//    for (int i = 0; i < MAX_SOCKETS; i++) {
//        // Sockets not logged in can be counted as empty (available)
//        if (!socket_details[i].logged_in) {
//            socket_details[i].client_socket = client_socket;
//            strncpy(socket_details[i].username, username, MAX_NAME_LENGTH);
//            socket_details[i].logged_in = true;
//            pthread_mutex_unlock(mutex);
//            printf("Success\n");
//            return 0;
//        }
//    }
//    pthread_mutex_unlock(mutex);
//    return -1;
//}
//
//// Logout of a socket to free up for a new connection
//int logout_socket_detail(SocketDetail* socket_details, int client_socket, pthread_mutex_t* mutex) {
//    pthread_mutex_lock(mutex);
//    for (int i = 0; i < MAX_SOCKETS; i++) {
//        if (socket_details[i].client_socket == client_socket) {
//            socket_details[i].logged_in = false;
//            pthread_mutex_unlock(mutex);
//            return 0;
//        }
//    }
//    pthread_mutex_unlock(mutex);
//    return -1;
//}
//
//// Attempt to find a socket for a given username
//int find_socket(SocketDetail* socket_details, char* username, pthread_mutex_t* mutex) {
//    pthread_mutex_lock(mutex);
//    for (int i = 0; i < MAX_SOCKETS; i++) {
//        if (strcmp(socket_details[i].username, username) == 0) {
//            pthread_mutex_lock(mutex);
//            return socket_details[i].client_socket;
//        }
//    }
//    printf("Couldn't find socket with username: %s\n", username);
//    pthread_mutex_unlock(mutex);
//    return -1;
//}

// Create new username if it doesn't already exist and mark as active. Return true for success, false for error
int login(int socket, const char args[3][MAX_ARG_LENGTH], char* name) {
    printf("%d LOGIN\n", socket);

    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", socket);
        send(socket, "false", 5, 0);
        return -1;
    }

    // Check that username is not longer than limit
    if (strlen(args[0]) > MAX_NAME_LENGTH) {
        fprintf(stderr, "%d Error: Name too long\n", socket);
        send(socket, "false", 5, 0);
        return -1;
    }

    // Check if the username exists in the player list
    bool userExists = false;
    for (int i = 0; i < gameData.player_count; i++) {
        if (strcmp(gameData.players[i].name, args[0]) == 0) {
            // Username exists, set the player as online
            gameData.players[i].online = true;
            gameData.players[i].socket = socket;
            userExists = true;
            break;
        }
    }

    // If the user does not exist, add them to the list
    if (!userExists) {
        if (gameData.player_count < MAX_PLAYERS) {
            strcpy(gameData.players[gameData.player_count].name, args[0]);
            gameData.players[gameData.player_count].online = true;
            gameData.players[gameData.player_count].socket = socket;
            gameData.player_count++; // Increment player count
        } else {
            fprintf(stderr, "%d Error: Player list is full\n", socket);
            send(socket, "false", 5, 0);
            return -1;
        }
    }

    // Save the updated GameData back to the JSON file
    if (save_to_json("game.json", &gameData)) {
        fprintf(stderr, "%d Error: Failed to save updated game data to JSON\n", socket);
        send(socket, "false", 5, 0);
        return -1;
    }

    strcpy(name, args[0]);
    send(socket, "true", 4, 0);
    return 0;
}

// Return all currently online users as json
int list(int socket, char args[3][255]) {
    printf("%d LIST\n", socket);

    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", socket);
        return -1;
    }

    // Create a JSON array to hold online players
    cJSON *onlinePlayers = cJSON_CreateArray();

    for (int i = 0; i < gameData.player_count; i++) {
        if (gameData.players[i].online && strcmp(gameData.players[i].name, args[0]) != 0) {
            // Add the online player's name to the JSON array
            cJSON_AddItemToArray(onlinePlayers, cJSON_CreateString(gameData.players[i].name));
        }
    }

    // Serialize the JSON array to a string
    char *jsonString = cJSON_PrintUnformatted(onlinePlayers);
    cJSON_Delete(onlinePlayers); // Free JSON object memory

    if (!jsonString) {
        fprintf(stderr, "%d Error: Failed to serialize JSON\n", socket);
        return -1;
    }

    // Send the JSON string to the client
    size_t jsonStringLength = strlen(jsonString);
    if (send(socket, jsonString, jsonStringLength, 0) == -1) {
        fprintf(stderr, "%d Error: Failed to send online players list\n", socket);
        free(jsonString);
        return -1;
    }

    free(jsonString); // Free the serialized JSON string
    return 0;
}

int challenge(int socket, char args[3][255]) {
    printf("%d CHALLENGE\n", socket);

    // Check challenger is not same as recipient
    if (strcmp(args[0], args[1]) == 0) {
        send(socket, "false", 5, 0);
        return -1;
    }

    // Get game data from json
    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", socket);
        return -1;
    }

    // Check there is not already a game between these two
    bool exists = false;
    for (int i = 0; i < gameData.game_count; i++) {
        if ((strcmp(gameData.games[i].player0, args[0]) == 0 && strcmp(gameData.games[i].player1, args[1]) == 0) ||
        (strcmp(gameData.games[i].player0, args[1]) == 0 && strcmp(gameData.games[i].player1, args[0]) == 0)) {
            exists = true;
            break;
        }
    }
    if (exists) {
        send(socket, "false", 5, 0);
        return -1;
    }

    // If not, assume acceptance and create a new game
    if (gameData.game_count >= MAX_GAMES) {
        fprintf(stderr, "%d Error: Maximum number of games reached\n", socket);
        send(socket, "false", 5, 0);
        return -1;
    }

    Game *newGame = &gameData.games[gameData.game_count];
    create_game(newGame, args[0], args[1]);
    gameData.game_count++;

    if (save_to_json(JSON_FILENAME, &gameData)) {
        fprintf(stderr, "%d Error: could not update json file\n", socket);
        send(socket, "false", 5, 0);
        return -1;
    }

    send(socket, "true", 4, 0);
    return 0;
}

// TODO
int accept_request(int socket, char args[3][255]) {
    printf("%d ACCEPT\n", socket);
    fprintf(stderr, "Not yet implemented\n");
    return 0;
}

// TODO
int decline(int socket, char args[3][255]) {
    printf("%d DECLINE\n", socket);
    fprintf(stderr, "Not yet implemented\n");
    return 0;
}

// TODO: This still doesn't work
int get_game(int socket, char args[3][255]) {
    printf("%d GAME\n", socket);
    GameData gameData;
    if (parse_json(&gameData, JSON_FILENAME)) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", socket);
        return -1;
    }

    // Find the game where players match args[0] and args[1] in any order
    int index = find_game(args[0], args[1], &gameData);

    if (index < 0) {
        // No game found
        fprintf(stderr, "%d Error: No game found for players %s and %s\n", socket, args[0], args[1]);
        send(socket, "false", 5, 0);
        return -1;
    }

    // Convert the found game to a JSON string
    char* json_string = game_to_json_string(&(gameData.games[index]));
    if (json_string == NULL) {
        fprintf(stderr, "%d Error: Failed to convert game to JSON\n", socket);
        send(socket, "false", 5, 0);
        return -1;
    }

    printf("Sending: %s\n", json_string);

    // Send the JSON string to the client
    if (send(socket, json_string, strlen(json_string), 0) == -1) {
        fprintf(stderr, "%d Error: Failed to send game state\n", socket);
        free(json_string);
        return -1;
    }

    free(json_string); // Free the JSON string
    return 0;
}

int get_all_games(int socket, char args[3][255]) {
    printf("%d LIST_GAMES\n", socket);

    GameData gameData;
    if (parse_json(&gameData, JSON_FILENAME)) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", socket);
        return -1;
    }

    // Create a JSON array to hold players
    cJSON *games = cJSON_CreateArray();


    for (int i = 0; i < gameData.game_count; i++) {
        if (strcmp(gameData.games[i].player0, args[0]) == 0) {
            // Add the other player's name to the JSON array
            cJSON_AddItemToArray(games, cJSON_CreateString(gameData.games[i].player1));
        } else if (strcmp(gameData.games[i].player1, args[0]) == 0) {
            // Add the other player's name to the JSON array
            cJSON_AddItemToArray(games, cJSON_CreateString(gameData.games[i].player0));
        }
    }

    // Serialize the JSON array to a string
    char *jsonString = cJSON_PrintUnformatted(games);
    cJSON_Delete(games); // Free JSON object memory

    if (!jsonString) {
        fprintf(stderr, "%d Error: Failed to serialize JSON\n", socket);
        return -1;
    }

    // Send the JSON string to the client
    size_t jsonStringLength = strlen(jsonString);
    if (send(socket, jsonString, jsonStringLength, 0) == -1) {
        fprintf(stderr, "%d Error: Failed to send online players list\n", socket);
        free(jsonString);
        return -1;
    }

    free(jsonString); // Free the serialized JSON string
    return 0;
}

// Move your pieces given args: [user1, user2, move]. Return new game state as json
int move(int socket, char args[3][255]) {
    printf("%d MOVE\n", socket);

    // Get game data
    GameData gameData;
    if (parse_json(&gameData, "game.json")) {
        fprintf(stderr, "%d Error: Failed to parse JSON\n", socket);
        send(socket, "false", 5, 0);
        return -1;
    }

    // Find game
    int index = find_game(args[0], args[1], &gameData);
    if (index < 0) {
        send(socket, "false", 5, 0);
        return -1;
    }

    int slot = convert_and_validate(args[2], 0, 12);

    if (slot < 0) {
        send(socket, "false", 5, 0);
        return -1;
    }

    // Check the correct person is trying to move
    if (! (gameData.games[index].current_state == MOVE_PLAYER_0 && strcmp(args[0], gameData.games[index].player0) == 0) &&
        ! (gameData.games[index].current_state == MOVE_PLAYER_1 && strcmp(args[0], gameData.games[index].player1) == 0)) {
        fprintf(stderr, "%d Error: Invalid attempt at move\n", socket);
        return -1;
    }

    // Perform the player's turn
    int has_won = play_turn(&gameData.games[index], slot - 1);
    if (has_won && gameData.games[index].current_state == MOVE_PLAYER_0) {
        gameData.games[index].current_state = WIN_PLAYER_0;
    } else if (has_won && gameData.games[index].current_state == MOVE_PLAYER_1) {
        gameData.games[index].current_state = WIN_PLAYER_1;
    }

    // Save the updated game data back to the JSON file
    if (save_to_json(JSON_FILENAME, &gameData) != 0) {
        fprintf(stderr, "%d Error: Failed to save updated game data to JSON\n", socket);
        return -1;
    }

    // Broadcast the updated game state to all players
    char* json_string = game_to_json_string(&gameData.games[index]);

    // Send the JSON string to the client
    if (send(socket, json_string, strlen(json_string), 0) == -1) {
        fprintf(stderr, "%d Error: Failed to send game state\n", socket);
        free(json_string);  // Free the string created by cJSON_Print
        return -1;
    }
    
    for(int i =0; i < gameData.player_count; i++){
        if(strcmp(gameData.players[i].name, gameData.games[index].player0) == 0 || strcmp(gameData.players[i].name, gameData.games[index].player1) == 0){
            if (send(gameData.players[i].socket, json_string, strlen(json_string), 0) == -1) {
                fprintf(stderr, "%d Error: Failed to send game state\n", gameData.players[i].socket);
                free(json_string);  // Free the string created by cJSON_Print
                return -1;
            }
        }
    }

//    int opponent_socket = find_socket(socket_details, args[1], socket_details_mutex);
//    printf("opponent socket found: %d", opponent_socket);
//
//    if (opponent_socket < 0) {
//        fprintf(stderr, "%d Info: Other user is not logged in, cannot send new state to them.\n", socket);
//        free(json_string);  // Free the string created by cJSON_Print
//        return 0;
//    }
//
//    // Send the JSON string to the other user
//    if (send(opponent_socket, json_string, strlen(json_string), 0) == -1) {
//        fprintf(stderr, "%d Error: Failed to send game state\n", socket);
//        free(json_string);  // Free the string created by cJSON_Print
//        return -1;
//    }


    // Free the JSON objects after use
    free(json_string);  // Free the string created by cJSON_Print
    return 0;
}

#endif //AWALEGAME_SERVER_H
