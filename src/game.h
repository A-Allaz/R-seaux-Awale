#ifndef R_SEAUX_AWALE_GAME_H
#define R_SEAUX_AWALE_GAME_H
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"

typedef struct Score {
    int player0;
    int player1;
} Score;

// The main game structure
typedef struct Game {
    char player0[255];
    char player1[255];
    int currentTurn;
    Score score;
    int board[12];
} Game;


int print_player_stats(Game *game, int player)
{
    if (player != 0 && player != 1) return 1;

    printf("====Stats for Player %d:====", player+1);

    if (player == 0)
    {
        printf("Name: %s", game->player0);
        printf("Score: %d", game->score.player0);
    }
    else
    {
        printf("Name: %s", game->player1);
        printf("Score: %d", game->score.player1);
    }

    printf("============================");
    return 0;
}


int save_game_state(const char *filename, Game *game) {
    // Create a JSON object
    cJSON *game_json = cJSON_CreateObject();

    // Add player names
    cJSON_AddStringToObject(game_json, "player0", game->player0);
    cJSON_AddStringToObject(game_json, "player1", game->player1);

    // Add current turn
    cJSON_AddNumberToObject(game_json, "currentTurn", game->currentTurn);

    // Add scores as a nested JSON object
    cJSON *score_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(score_json, "player0", game->score.player0);
    cJSON_AddNumberToObject(score_json, "player1", game->score.player1);
    cJSON_AddItemToObject(game_json, "score", score_json);

    // Add board state as a JSON array
    cJSON *board_array = cJSON_CreateIntArray(game->board, 12);
    cJSON_AddItemToObject(game_json, "boardState", board_array);

    // Convert JSON object to string
    char *json_string = cJSON_Print(game_json);

    // Write JSON string to file
    FILE *file = fopen(filename, "w");
    if (file) {
        fprintf(file, "%s", json_string);
        fclose(file);
    } else {
        perror("Error opening file");
        return 1;
    }

    // Cleanup
    cJSON_Delete(game_json);
    free(json_string);
    return 0;
}


int load_game_state(const char *filename, Game *game) {
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
    cJSON *game_json = cJSON_Parse(data);
    if (game_json == NULL) {
        printf("Error parsing JSON data\n");
        free(data);
        return 1;
    }

    // Extract player names
    cJSON *player0_json = cJSON_GetObjectItemCaseSensitive(game_json, "player0");
    cJSON *player1_json = cJSON_GetObjectItemCaseSensitive(game_json, "player1");
    strcpy(game->player0, player0_json->valuestring);
    strcpy(game->player1, player1_json->valuestring);

    // Extract current turn
    game->currentTurn = cJSON_GetObjectItemCaseSensitive(game_json, "currentTurn")->valueint;

    // Extract scores
    cJSON *score_json = cJSON_GetObjectItemCaseSensitive(game_json, "score");
    game->score.player0 = cJSON_GetObjectItemCaseSensitive(score_json, "player0")->valueint;
    game->score.player1 = cJSON_GetObjectItemCaseSensitive(score_json, "player1")->valueint;

    // Extract board state
    cJSON *board_array = cJSON_GetObjectItemCaseSensitive(game_json, "boardState");
    for (int i = 0; i < 12; i++) {
        game->board[i] = cJSON_GetArrayItem(board_array, i)->valueint;
    }

    // Cleanup
    cJSON_Delete(game_json);
    free(data);
    return 0;
}