#ifndef R_SEAUX_AWALE_GAME_H
#define R_SEAUX_AWALE_GAME_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"


typedef enum {
    MOVE_PLAYER_0,
    MOVE_PLAYER_1,
    WIN_PLAYER_0,
    WIN_PLAYER_1,
} GAME_STATE;

typedef struct {
    int player0;
    int player1;
} Score;

// The main game structure
typedef struct {
    char player0[255];
    char player1[255];
    GAME_STATE current_state;
    Score score;
    int board[12];
} Game;

int init_score(Game* game) {
    game->score.player0 = 0;
    game->score.player1 = 0;
    return 0;
}

int init_game(Game *game, const char* player0, const char* player1) {
    strcpy(game->player0, player0);
    strcpy(game->player1, player1);
    game->current_state = MOVE_PLAYER_0;
    init_score(game);
    for (int i = 0; i < 12; i++){
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
    for(int i = 0; i < 6; i++) {
        if (game->board[((game->current_state + 1) % 2) * 6 + i] != 0) { // Checks the enemy's side
            hasPebbles = 1;
            break;
        }
    }

    if(!hasPebbles) {
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

int print_player_stats(Game *game, int player)
{
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
    FILE *file = fopen("game.json", "w");
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

// Function to read the content of a file into a string
char* read_json_to_string(const char* filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Unable to open file %s\n", filename);
        return NULL;
    }

    // Get the file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    // Allocate memory for the file content
    char *content = (char *) malloc(file_size + 1);
    if (!content) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        fclose(file);
        return NULL;
    }

    // Read the file into the buffer
    fread(content, 1, file_size, file);
    content[file_size] = '\0'; // Null-terminate the string

    fclose(file);
    return content;
}

// Dynamically allocates memory to list of players
char** get_online_players(int* count) {
    // Initialize count to zero
    *count = 0;

    // Read the file content
    char* file_content = read_json_to_string("game.json");
    if (!file_content) {
        return NULL;
    }

    // Parse the JSON content
    cJSON* json = cJSON_Parse(file_content);
    free(file_content); // Free the file content buffer

    if (!json) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        return NULL;
    }

    // Extract the "active_players" array
    cJSON* active_players = cJSON_GetObjectItemCaseSensitive(json, "active_players");
    if (!cJSON_IsArray(active_players)) {
        fprintf(stderr, "Error: 'active_players' is not an array or does not exist\n");
        cJSON_Delete(json);
        return NULL;
    }

    // Get the number of active players
    *count = cJSON_GetArraySize(active_players);

    // Allocate memory for the list of player names
    char** player_list = (char**) malloc(*count * sizeof(char*));
    if (!player_list) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        cJSON_Delete(json);
        return NULL;
    }

    // Iterate through the array and copy player names
    for (int i = 0; i < *count; i++) {
        cJSON* player = cJSON_GetArrayItem(active_players, i);
        if (cJSON_IsString(player) && player->valuestring) {
            player_list[i] = (char*) malloc(255 * sizeof(char));
            if (!player_list[i]) {
                fprintf(stderr, "Error: Memory allocation failed for player name\n");

                // Free already allocated memory
                for (int j = 0; j < i; j++) {
                    free(player_list[j]);
                }
                free(player_list);
                cJSON_Delete(json);
                return NULL;
            }
            strncpy(player_list[i], player->valuestring, 254);
            player_list[i][254] = '\0'; // Ensure null-termination
        } else {
            fprintf(stderr, "Warning: Invalid player name found\n");
            player_list[i] = NULL;
        }
    }

    // Clean up the original JSON object
    cJSON_Delete(json);

    return player_list;
}

int save_game_state(const char *filename, Game *game) {
    FILE *file = fopen("game.json", "r+");

    if (file == NULL) {
        printf("File 'game.json' does not exist. Creating it now...\n");

        if (createGameFile()) {
            printf("CRITICAL: Error creating game.json");
            exit(1);
        }

        // Attempt to reopen
        file = fopen(filename, "r+");
        if (!file) {
            perror("Error opening file");
            exit(1);
        }
    }

    // Read the entire file content into memory
    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    // Parse the existing JSON data
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        printf("Error parsing JSON data\n");
        free(data);
        return 2;
    }

    // Extract the games array from the JSON
    cJSON *games_array = cJSON_GetObjectItemCaseSensitive(json, "games");
    if (!cJSON_IsArray(games_array)) {
        // If the "games" array doesn't exist, create it
        games_array = cJSON_CreateArray();
        cJSON_AddItemToObject(json, "games", games_array);
    }

    // Flag to check if the game for the two users already exists
    int game_found = 0;
    cJSON *game_json = NULL;

    // Iterate through the games array and check if the game exists for the given users
    cJSON_ArrayForEach(game_json, games_array) {
        cJSON *player0_json = cJSON_GetObjectItemCaseSensitive(game_json, "player0");
        cJSON *player1_json = cJSON_GetObjectItemCaseSensitive(game_json, "player1");

        // Check if the players match
        if (player0_json && player1_json &&
            strcmp(player0_json->valuestring, game->player0) == 0 &&
            strcmp(player1_json->valuestring, game->player1) == 0) {
            game_found = 1; // Game found, break out of the loop
            break;
        }
    }

    // If the game doesn't exist, create a new entry
    if (!game_found) {
        // Create a new game JSON object
        game_json = cJSON_CreateObject();

        // Add player names
        cJSON_AddStringToObject(game_json, "player0", game->player0);
        cJSON_AddStringToObject(game_json, "player1", game->player1);

        // Add current turn
        cJSON_AddNumberToObject(game_json, "currentState", game->current_state);

        // Add scores
        cJSON *score_json = cJSON_CreateObject();
        cJSON_AddNumberToObject(score_json, "player0", game->score.player0);
        cJSON_AddNumberToObject(score_json, "player1", game->score.player1);
        cJSON_AddItemToObject(game_json, "score", score_json);

        // Add board state
        cJSON *board_array = cJSON_CreateIntArray(game->board, 12);
        cJSON_AddItemToObject(game_json, "board", board_array);

        // Add the new game object to the "games" array
        cJSON_AddItemToArray(games_array, game_json);
    } else {
        // If the game is found, just update the existing game JSON object

        // Update current turn
        cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(game_json, "currentState"), game->current_state);

        // Update scores
        cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(game_json, "score")->child, game->score.player0);
        cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(game_json, "score")->child->next, game->score.player1);

        // Update board state
        cJSON *board_array = cJSON_CreateIntArray(game->board, 12);
        cJSON_ReplaceItemInObjectCaseSensitive(game_json, "board", board_array);
    }

    // Convert the updated JSON object back to a string
    char *json_string = cJSON_Print(json);

    // Write the updated JSON string to the file
    file = fopen(filename, "w");
    if (file) {
        fprintf(file, "%s", json_string);
        fclose(file);
    } else {
        perror("Error opening file for writing");
        cJSON_Delete(json);
        free(json_string);
        return 3;
    }

    // Cleanup
    cJSON_Delete(json);
    free(data);
    free(json_string);
    return 0;
}

// Load the game between user0 and user1
int load_game_state(const char *filename, Game *game, const char *user0, const char *user1) {
    // Open the file and read its content
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *data = malloc(length + 1);
    fread(data, 1, length, file);
    data[length] = '\0';
    fclose(file);

    // Parse JSON data
    cJSON *json = cJSON_Parse(data);
    if (json == NULL) {
        printf("Error parsing JSON data\n");
        free(data);
        return 2;
    }

    // Extract the games array
    cJSON *games_array = cJSON_GetObjectItemCaseSensitive(json, "games");
    if (!cJSON_IsArray(games_array)) {
        printf("Error: games array not found\n");
        cJSON_Delete(json);
        free(data);
        return 3;
    }

    // Iterate through the games array and find the matching game
    int game_found = 0;
    cJSON *game_json = NULL;
    cJSON_ArrayForEach(game_json, games_array) {
        cJSON *player0_json = cJSON_GetObjectItemCaseSensitive(game_json, "player0");
        cJSON *player1_json = cJSON_GetObjectItemCaseSensitive(game_json, "player1");

        // Check if the players match
        if (player0_json && player1_json &&
            strcmp(player0_json->valuestring, user0) == 0 &&
            strcmp(player1_json->valuestring, user1) == 0) {
            game_found = 1;
            break; // Game found, no need to continue searching
        }
    }

    if (!game_found) {
        printf("Game not found for the given players: %s, %s\n", user0, user1);
        cJSON_Delete(json);
        free(data);
        return 4; // Return an error if game is not found
    }

    // Now, load the game data into the `game` structure
    // Extract the current turn
    game->current_state = cJSON_GetObjectItemCaseSensitive(game_json, "currentState")->valueint;

    // Extract scores
    cJSON *score_json = cJSON_GetObjectItemCaseSensitive(game_json, "score");
    game->score.player0 = cJSON_GetObjectItemCaseSensitive(score_json, "player0")->valueint;
    game->score.player1 = cJSON_GetObjectItemCaseSensitive(score_json, "player1")->valueint;

    // Extract board state
    cJSON *board_array = cJSON_GetObjectItemCaseSensitive(game_json, "board");
    for (int i = 0; i < 12; i++) {
        game->board[i] = cJSON_GetArrayItem(board_array, i)->valueint;
    }

    // Cleanup
    cJSON_Delete(json);
    free(data);
    return 0;
}

// Load game instance given a pair of usernames, returns NULL if error (no memory allocated)
Game* load_game(const char* user0, const char* user1) {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return NULL;
    }

    int res = load_game_state("game.json", game, user0, user1);
    if (res == 1) {
        printf("Error opening file");
    } else if (res == 2) {
        printf("Error parsing JSON file");
    } else if (res == 3) {
        printf("Game with these users does not exist");
    } else {
        return game;
    }

    free(game);
    return NULL;
}

// Create game instance given both usernames
Game* create_game(const char* user0, const char* user1) {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE); // Terminate the program with a failure status
    }

    // Decide who starts here
    if (rand() % 2) {
        init_game(game,user0,user1);
    } else {
        init_game(game,user1,user0);
    }

    return game;
}
