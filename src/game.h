//
// Defines shared resources for representing a game.
// To be used by both client and server
//

#ifndef R_SEAUX_AWALE_GAME_H

#define R_SEAUX_AWALE_GAME_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "cJSON.h"

#define MAX_PLAYERS 100
#define MAX_GAMES 100
#define MAX_NAME_LENGTH 32
#define MAX_BIO_LENGTH 80
#define BOARD_SIZE 12
#define JSON_FILENAME "game.json"


typedef enum {
    MOVE_PLAYER_0,
    MOVE_PLAYER_1,
    WIN_PLAYER_0,
    WIN_PLAYER_1,
} GAME_STATE;

// Represent the score of a game
typedef struct {
    int player0;
    int player1;
} Score;

// Represent a single game
typedef struct {
    char player0[MAX_NAME_LENGTH + 1];  // Allow for '/0'
    char player1[MAX_NAME_LENGTH + 1];  // Allow for '/0'
    GAME_STATE current_state;
    Score score;
    int board[BOARD_SIZE];
} Game;

// Represent a single player
typedef struct {
    char name[MAX_NAME_LENGTH + 1];
    bool online;
//    char bio[MAX_BIO_LENGTH + 1];
//    int score;
//    char received_requests[MAX_NAME_LENGTH + 1][MAX_PLAYERS - 1];
//    char sent_requests[MAX_NAME_LENGTH + 1][MAX_PLAYERS - 1];
} Player;

// Represent the entire game data
typedef struct {
    Player players[MAX_PLAYERS];
    int player_count;
    Game games[MAX_GAMES];
    int game_count;
} GameData;

int init_score(Game* game) {
    game->score.player0 = 0;
    game->score.player1 = 0;
    return 0;
}

int init_game(Game *game, const char* player0, const char* player1) {
    strncpy(game->player0, player0, MAX_NAME_LENGTH);
    strncpy(game->player1, player1, MAX_NAME_LENGTH);
    game->player0[MAX_NAME_LENGTH] = '\0';
    game->player1[MAX_NAME_LENGTH] = '\0';

    game->current_state = MOVE_PLAYER_0;
    init_score(game);
    for (int i = 0; i < 12; i++) {
        game->board[i] = 4;
    }
    return 1;
}

int move_pebbles(Game* game, int slot) {
    const int pebbles = game->board[slot];

    for (int i = 0; i < pebbles + 1;){
        if((slot + i + 1) % 12 != slot){
            game->board[(slot + i) % 12]++;
            i++;
        }
    }
    game->board[slot] = 0;

    return pebbles;
}

int compute_score(Game* game, int slot, int pebbles) {
    int i = 0;
    GAME_STATE current = game->current_state;

    int lastSlot = (slot + pebbles - i) % 12;
    int slotScore = game->board[lastSlot];

    while (slotScore == 2 || slotScore == 3) {
        if (current == MOVE_PLAYER_0 && lastSlot >= 6) { // in player1's side
            game->score.player0 += slotScore;
            game->board[lastSlot] = 0;
        } else if (current == MOVE_PLAYER_1 && lastSlot < 6) { // in player0's side
            game->score.player1 += slotScore;
            game->board[lastSlot] = 0;
        }

        i++;

        lastSlot = (slot + pebbles - i) % 12;
        slotScore = game->board[lastSlot];
    }

    return 0;
}

int play_turn(Game* game, int slot) {
    // Checking victory conditions : if returns 1, the current player has won

    int pebbles = move_pebbles(game, slot);

    // Victory if opponent has no pebbles in its camp
    int hasPebbles = 0; // Only for enemy player
    for (int i = 0; i < 6; i++) {
        if (game->board[((game->current_state + 1) % 2) * 6 + i] != 0) { // Checks the enemy's side
            hasPebbles = 1;
            break;
        }
    }

    if (!hasPebbles) {
        return 1;
    }

    compute_score(game, slot, pebbles);

    // Victory if more than half the available points
    if ((game->current_state == 0 && game->score.player0 > 24) ||
        (game->current_state == 1 && game->score.player0 > 24)) {
        return 1;
    }

    // switch turn
    game->current_state = (game->current_state + 1) % 2;
    return 0;
}

int print_board_state(Game* game) {
    printf("========= Board State: =========\n\n");

    printf("\033[0;37m");
    printf("\t(1)\t\t(2)\t\t(3)\t\t(4)\t\t(5)\t\t(6)\n");

    if (game->current_state == 0) {
        printf("\033[0;92m");
    } else {
        printf("\033[0;91m");
    }

    for(int i = 0; i < 6; i++) {
        printf("|\t%d\t", game->board[i]);
        if(i == 5) {
            printf("| -- %s's side --\n", game->player0);
        }
    }

    if (game->current_state == 0) {
        printf("\033[0;91m");
    } else {
        printf("\033[0;92m");
    }

    for(int i = 11; i > 5; i--) {
        printf("|\t%d\t",game->board[i]);
        if(i == 6) {
            printf("| -- %s's side --\n", game->player1);
        }
    }

    printf("\033[0;37m");
    printf("\t(12)\t\t(11)\t\t(10)\t\t(9)\t\t(8)\t\t(7)\n\n");
    printf("\033[0;39m");
}

int print_player_stats(Game *game, int player) {
    if (player != 0 && player != 1) return 1;

    printf("===== Stats for Player %d: =====\n", player+1);

    if (player == 0)
    {
        printf("Name: %s\n", game->player0);
        printf("Score: %d\n", game->score.player0);
    }
    else
    {
        printf("Name: %s\n", game->player1);
        printf("Score: %d\n", game->score.player1);
    }

    printf("================================\n");
    return 0;
}

// Create game file if it doesn't exist already
int createGameFile() {
    // Create the root JSON object
    cJSON *root = cJSON_CreateObject();
    if (!root) {
        fprintf(stderr, "Error creating JSON object\n");
        return 1;
    }

    // Add empty arrays to the JSON object
    cJSON *players = cJSON_CreateArray();
    cJSON *active_players = cJSON_CreateArray();
    cJSON *games = cJSON_CreateArray();

    if (!players || !active_players || !games) {
        fprintf(stderr, "Error creating JSON arrays\n");
        cJSON_Delete(root);
        return 1;
    }

    cJSON_AddItemToObject(root, "players", players);
    cJSON_AddItemToObject(root, "active_players", active_players);
    cJSON_AddItemToObject(root, "games", games);

    // Convert the JSON object to a string
    char *json_string = cJSON_Print(root);
    if (!json_string) {
        fprintf(stderr, "Error converting JSON object to string\n");
        cJSON_Delete(root);
        return 1;
    }

    // Write the JSON string to a file
    FILE *file = fopen(JSON_FILENAME, "w");
    if (!file) {
        fprintf(stderr, "Error opening file for writing\n");
        free(json_string);
        cJSON_Delete(root);
        return 1;
    }

    fprintf(file, "%s", json_string);
    fclose(file);

    // Free allocated memory
    free(json_string);
    cJSON_Delete(root);

    return 0;
}

// Parse the JSON file into a GameData struct
int parse_json(GameData* gameData, const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    // Read the file into a buffer
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *json_str = (char *) malloc(length + 1);
    fread(json_str, 1, length, file);
    json_str[length] = '\0';
    fclose(file);

    // Parse the JSON string
    cJSON *json = cJSON_Parse(json_str);
    free(json_str);

    if (!json) {
        printf("Error parsing JSON: %s\n", cJSON_GetErrorPtr());
        return 1;
    }

    // Parse players
    cJSON *players = cJSON_GetObjectItem(json, "players");
    if (players && cJSON_IsArray(players)) {
        gameData->player_count = cJSON_GetArraySize(players);
        for (int i = 0; i < gameData->player_count; i++) {
            cJSON *player = cJSON_GetArrayItem(players, i);
            if (player && cJSON_IsObject(player)) {
                cJSON *name = cJSON_GetObjectItem(player, "name");
                cJSON *online = cJSON_GetObjectItem(player, "online");

                if (name && cJSON_IsString(name) && online && cJSON_IsBool(online)) {
                    strncpy(gameData->players[i].name, name->valuestring, MAX_NAME_LENGTH);
                    gameData->players[i].online = online->valueint;
                }
            }
        }
    }

    // Parse games
    cJSON *games = cJSON_GetObjectItem(json, "games");
    if (games && cJSON_IsArray(games)) {
        gameData->game_count = cJSON_GetArraySize(games);
        for (int i = 0; i < gameData->game_count; i++) {
            cJSON *game = cJSON_GetArrayItem(games, i);

            if (game && cJSON_IsObject(game)) {
                cJSON *player0 = cJSON_GetObjectItem(game, "player0");
                cJSON *player1 = cJSON_GetObjectItem(game, "player1");
                cJSON *current_state = cJSON_GetObjectItem(game, "currentState");

                cJSON *score = cJSON_GetObjectItem(game, "score");
                cJSON *board = cJSON_GetObjectItem(game, "board");

                if (player0 && cJSON_IsString(player0)) {
                    strncpy(gameData->games[i].player0, player0->valuestring, MAX_NAME_LENGTH);
                }
                if (player1 && cJSON_IsString(player1)) {
                    strncpy(gameData->games[i].player1, player1->valuestring, MAX_NAME_LENGTH);
                }
                if (current_state && cJSON_IsNumber(current_state)) {
                    gameData->games[i].current_state = current_state->valueint;
                }

                if (score && cJSON_IsObject(score)) {
                    cJSON *score0 = cJSON_GetObjectItem(score, "player0");
                    cJSON *score1 = cJSON_GetObjectItem(score, "player1");

                    if (score0 && cJSON_IsNumber(score0)) {
                        gameData->games[i].score.player0 = score0->valueint;
                    }
                    if (score1 && cJSON_IsNumber(score1)) {
                        gameData->games[i].score.player1 = score1->valueint;
                    }
                }

                if (board && cJSON_IsArray(board)) {
                    int board_size = cJSON_GetArraySize(board);
                    for (int j = 0; j < board_size && j < BOARD_SIZE; j++) {
                        cJSON *cell = cJSON_GetArrayItem(board, j);
                        if (cell && cJSON_IsNumber(cell)) {
                            gameData->games[i].board[j] = cell->valueint;
                        }
                    }
                }
            }
        }
    }

    cJSON_Delete(json);
    return 0;
}

// Save a given GameData struct into the JSON file
int save_to_json(const char *filename, const GameData *data) {
    cJSON *json = cJSON_CreateObject();

    // Add players array
    cJSON *players = cJSON_CreateArray();
    for (int i = 0; i < data->player_count; i++) {
        cJSON *player = cJSON_CreateObject();
        cJSON_AddStringToObject(player, "name", data->players[i].name);
        cJSON_AddBoolToObject(player, "online", data->players[i].online);
        cJSON_AddItemToArray(players, player);
    }
    cJSON_AddItemToObject(json, "players", players);

    // Add games array
    cJSON *games = cJSON_CreateArray();
    for (int i = 0; i < data->game_count; i++) {
        cJSON *game = cJSON_CreateObject();
        cJSON_AddStringToObject(game, "player0", data->games[i].player0);
        cJSON_AddStringToObject(game, "player1", data->games[i].player1);
        cJSON_AddNumberToObject(game, "currentState", data->games[i].current_state);

        cJSON *score = cJSON_CreateObject();
        cJSON_AddNumberToObject(score, "player0", data->games[i].score.player0);
        cJSON_AddNumberToObject(score, "player1", data->games[i].score.player1);
        cJSON_AddItemToObject(game, "score", score);

        cJSON *board = cJSON_CreateIntArray(data->games[i].board, BOARD_SIZE);
        cJSON_AddItemToObject(game, "board", board);

        cJSON_AddItemToArray(games, game);
    }
    cJSON_AddItemToObject(json, "games", games);

    // Serialize JSON to string
    char *json_str = cJSON_Print(json);

    // Write JSON string to file
    FILE *file = fopen(filename, "w");
    if (file) {
        fputs(json_str, file);
        fclose(file);
    } else {
        perror("Error writing to file");
        return -1;
    }

    // Cleanup
    cJSON_Delete(json);
    free(json_str);
    return 0;
}

// Load game instance given a pair of usernames, returns NULL if error (no memory allocated)
int load_game(const char* user0, const char* user1) {
    // Load the game data from the JSON file
    GameData gameData;
    if (parse_json(&gameData, JSON_FILENAME)) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        return -1;
    }

    // Check if the game already exists
    for (int i = 0; i < gameData.game_count; i++) {
        if (strcmp(gameData.games[i].player0, user0) == 0 && strcmp(gameData.games[i].player1, user1) == 0) {
            return i;
        }
    }

    // If the game was not found, add it to the list
    fprintf(stderr, "Error: Game not found.\n");
    return -1;
}

// Same as init_game but randomises the starting player
int create_game(Game* game, const char* user0, const char* user1) {
    // Decide who starts here
    if (rand() % 2) {
        init_game(game, user0, user1);
    } else {
        init_game(game, user1, user0);
    }

    return 0;
}

#endif