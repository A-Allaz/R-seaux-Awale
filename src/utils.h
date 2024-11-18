#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "game.h"

void clrscr()
{
    system("@cls||clear");
}

int interactive_play(Game *game) {
    int slot;
    while(game->score.player0 < 25 && game->score.player1 < 25) {
        clrscr();
        print_player_stats(game, 0);
        print_player_stats(game, 1);
        print_board_state(game);

        if (game->current_state == MOVE_PLAYER_0){
            printf("%s'S TURN\t", game->player0);
        } else {
            printf("%s'S TURN\t", game->player1);
        }
        printf("\033[0;93m"); // Yellow
        printf("Surrender (0)\n");
        printf("\033[0;94m"); // Blue
        printf("=== Choose a slot (1 to 12): ===\n");
        printf("\033[0;39m"); // White
        
        while (1) {
            if (scanf("%d", &slot) != 1 || slot < 0 || slot > 12) {
            // Clear the invalid input from stdin buffer
            while (getchar() != '\n');
            printf("Invalid input! Please enter a number between 0 and 12.\n");
            continue;  // Skip the rest of the loop
            }

            // Case : surrender
            if (slot == 0){
                int surrender;
                printf("SURRENDER ? (0: NO / 1: YES):\n");
                while (getchar() != '\n'); // Flush the input buffer
                while (1) {
                    if (scanf("%d", &surrender) == 1 && (surrender == 0 || surrender == 1)) {
                        break;
                    } else {
                        while (getchar() != '\n');
                        printf("Wrong input, Please enter 0 or 1\n");
                        printf("SURRENDER ? (0: NO / 1: YES):\n");
                    }
                }
                if (surrender == 1) {
                    // Change game state to over
                    return 1;
                } else if (surrender == 0) {
                    break;
                }
            }

            if (slot >= 1) {
                if (play_turn(game, slot - 1)) {
                    printf("Game is over\n");
                } else {
                    printf("Game is ON\n");
                }
                break;
            }
        }

        // printf("You selected slot %d.\n", slot);
    }

    return 0;
}

// Check a given slot is valid for make_move()
int convert_and_validate(const char *str) {
    // Convert the string to a long integer using strtol
    char *endptr;
    long result = strtol(str, &endptr, 10);  // base 10

    // Check if the entire string was a valid number
    if (*endptr != '\0') {
        printf("Error: The string contains non-numeric characters.\n");
        return -1;  // Return -1 to indicate invalid conversion
    }

    // Check for range: the number must be between 0 and 12
    if (result < 0 || result > 12) {
        printf("Error: The number must be between 0 and 12.\n");
        return -1;
    }

    // Check for overflow/underflow conditions
    if (result == LONG_MIN || result == LONG_MAX) {
        printf("Error: Overflow or underflow occurred.\n");
        return -1;
    }

    return (int) result;  // Return the converted value as an integer
}

// Convert a given game object to a json string
char* game_to_json_string(Game* game) {
    // Create the JSON object for the game
    cJSON *game_json = cJSON_CreateObject();

    // Add player names to the JSON
    cJSON_AddStringToObject(game_json, "player0", game->player0);
    cJSON_AddStringToObject(game_json, "player1", game->player1);

    // Add the current game state to the JSON
    cJSON_AddNumberToObject(game_json, "currentState", game->current_state);

    // Add the score to the JSON
    cJSON *score_json = cJSON_CreateObject();
    cJSON_AddNumberToObject(score_json, "player0", game->score.player0);
    cJSON_AddNumberToObject(score_json, "player1", game->score.player1);
    cJSON_AddItemToObject(game_json, "score", score_json);

    // Add the board state to the JSON
    cJSON *board_json = cJSON_CreateArray();
    for (int i = 0; i < 12; i++) {
        cJSON_AddItemToArray(board_json, cJSON_CreateNumber(game->board[i]));
    }
    cJSON_AddItemToObject(game_json, "board", board_json);

    // Convert the JSON object to a string
    char* json = cJSON_Print(game_json);  // Converts to string, requires free()
    cJSON_Delete(game_json);  // Free JSON object

    return json;
}