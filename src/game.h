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

int init_score(Game* game){
    game->score.player0 = 0;
    game->score.player1 = 0;
    return 0;
}

int init_game(Game *game, char* player0, char* player1) {
    strcpy(game->player0, player0);
    strcpy(game->player1, player1);
    game->currentTurn = 0;
    init_score(game);
    for (int i = 0; i < 12; i++){
        game->board[i] = 4;
    }
    return 1;
}

int move_pebbles(Game* game, int slot){
    printf("move_pebbles\n");
    const int pebbles = game->board[slot];
    printf("pebbles: %d \n", pebbles);
    for (int i = 0; i < pebbles + 1;){
        if((slot + i + 1) % 12 != slot){
            game->board[(slot + i) % 12]++;
            i++;
        }
    }
    game->board[slot] = 0;

    return pebbles;
}

int compute_score(Game* game, int slot, int pebbles){
    printf("compute-score\n");
    int i = 0;
    int current = game->currentTurn;

    int lastSlot = (slot + pebbles - i) % 12;
    printf("lastSlot: %d \n", lastSlot);
    int slotScore = game->board[lastSlot];
    printf("slotScore: %d\n", slotScore);

    while (slotScore == 2 || slotScore == 3) {
        if (current == 0 && lastSlot >= 6) { // in player1's side
            game->score.player0 += slotScore;
            game->board[lastSlot] = 0;
        } else if (current == 1 && lastSlot < 6) { // in player0's side
            game->score.player1 += slotScore;
            game->board[lastSlot] = 0;
        }

        i++;

        lastSlot = (slot + pebbles - i) % 12;
        slotScore = game->board[lastSlot];
    }

    return 0;
}

int play_turn(Game* game, int slot){
    printf("play_turn\n");
    int pebbles = move_pebbles(game, slot);
    printf("pebbles = %d\n", pebbles);
    compute_score(game, slot, pebbles);

    // check victory : if returns 1, the current player won the game through points
    if((game->currentTurn == 0 && game->score.player0 > 24) || (game->currentTurn == 1 && game->score.player0 > 24)){
        return 1;
    }

    // switch turn
    game->currentTurn = (game->currentTurn + 1) % 2;
    return 0;
}

int print_board_state(Game* game) {
    printf("======= Board State: =======\n");
    printf("\t(1)\t\t(2)\t\t(3)\t\t(4)\t\t(5)\t\t(6)\n");
    for(int i = 0; i < 6; i++) {
        printf("|\t%d\t", game->board[i]);
        if(i == 5) {
            printf("| -- player 1's side --\n");
        }
    }
    for(int i = 6; i < 12; i++) {
        printf("|\t%d\t",game->board[i]);
        if(i == 11) {
            printf("| -- player 2's side --\n");
        }
    }
    printf("\t(7)\t\t(8)\t\t(9)\t\t(10)\t\t(11)\t\t(12)\n");
}

int print_player_stats(Game *game, int player)
{
    if (player != 0 && player != 1) return 1;

    printf("====Stats for Player %d:====\n", player+1);

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

    printf("============================\n");
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
    cJSON_AddItemToObject(game_json, "board", board_array);

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
    cJSON *board_array = cJSON_GetObjectItemCaseSensitive(game_json, "board");
    for (int i = 0; i < 12; i++) {
        game->board[i] = cJSON_GetArrayItem(board_array, i)->valueint;
    }

    // Cleanup
    cJSON_Delete(game_json);
    free(data);
    return 0;
}