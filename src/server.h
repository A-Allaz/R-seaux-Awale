//
// Defines functions for the client
//

#ifndef AWALEGAME_SERVER_H
#define AWALEGAME_SERVER_H

#include "utils.h"
#include "network.h"


/**
 * @brief Handles a login request by validating the username, updating player status, and saving changes.
 *
 * @param socket The client socket.
 * @param args args[0] = The username the client is trying to log in with.
 * @param name The string where the logged-in player's username will be stored if successful.
 *
 * @return int Returns 0 on successful login, or -1 if there is an error.
 */
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


/**
 * @brief Retrieves and sends a list of online players, excluding a given username.
 *
 * @param socket The client socket.
 * @param args args[0] = The username of the client requesting the list of online players.
 *
 * @return int Returns 0 on success, or -1 if there is an error.
 */
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


/**
 * @brief Handles a challenge request to create a new game between two players.
 *
 * @param socket The client socket.
 * @param args args[0] = The username of the player issuing the challenge,
 * args[1] = The username of the player being challenged.
 *
 * @return int Returns 0 on success, or -1 if there is an error.
 *
 */
// TODO: currently just creates a new game, should add a request to the person being challenged
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


/**
 * @brief Retrieves the game state for the specified players and sends it to the client.
 *
 * @param socket The client socket.
 * @param args args[0] = Player0's username, args[1] = Player1's username.
 *
 * @return int Returns 0 on success, or -1 on failure.
 *
 * @details If no game is found, or if any error occurs (e.g., JSON parsing or sending),
 * an error message is logged, and "false" is sent to the client.
 */
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

    // Send the JSON string to the client
    if (send(socket, json_string, strlen(json_string), 0) == -1) {
        fprintf(stderr, "%d Error: Failed to send game state\n", socket);
        free(json_string);
        return -1;
    }

    free(json_string); // Free the JSON string
    return 0;
}


/**
 * @brief Finds all the games a given user has and sends a list of the opponent names to the client.
 *
 * @param socket The client socket.
 * @param args args[0] = Player's username.
 *
 * @return int Returns 0 on success, or -1 on failure.
 *
 * @details If any error occurs (e.g., JSON parsing or sending),
 * an error message is logged, and "false" is sent to the client.
 */
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


/**
 * @brief Handles a player's move in the game, updates the game state, and returns the updated state.
 *
 * @param socket The client socket.
 * @param args args[0] = The player's username, args[1] = The opponent's username,
 * args[2] = The move (slot number) as a string.
 *
 * @return int Returns 0 on success, or -1 on failure.
 */
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

    printf("Slot before conversion: %s\n", args[2]);

    int slot = convert_and_validate(args[2], 0, 12);

    printf("Slot: %d\n", slot);

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

    // Check if this is a surrender
    if (slot == 0) {
        if (gameData.games[index].current_state == MOVE_PLAYER_0) {
            gameData.games[index].current_state = WIN_PLAYER_1;
        } else if (gameData.games[index].current_state == MOVE_PLAYER_1) {
            gameData.games[index].current_state = WIN_PLAYER_0;
        }
    } else {
        // Perform the player's turn
        int has_won = play_turn(&gameData.games[index], slot - 1);
        if (has_won && gameData.games[index].current_state == MOVE_PLAYER_0) {
            gameData.games[index].current_state = WIN_PLAYER_0;
        } else if (has_won && gameData.games[index].current_state == MOVE_PLAYER_1) {
            gameData.games[index].current_state = WIN_PLAYER_1;
        }
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

    // Free the JSON objects after use
    free(json_string);  // Free the string created by cJSON_Print
    return 0;
}


#endif //AWALEGAME_SERVER_H
