//
// Created by Christopher Wilson on 08/11/2024.
//


#include "game.h"
#include "utils.h"

int interactive_play(Game *game);

int main() {
    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE); // Terminate the program with a failure status
    }

    init_game(game,"Adam","Chris");
    interactive_play(game);
    free(game);
    // load_game_state("game.json", game);
    // print_player_stats(game, 0);
    // print_player_stats(game, 1);
    // save_game_state("game.json", game);
    return 0;
}

int interactive_play(Game *game) {
    int slot;
    while(game->score.player0 < 25 && game->score.player1 < 25) {
        clrscr();
        if(play_turn(game, slot - 1)) {
            printf("Game is over\n");
        } else {
            printf("Game is ON\n");
        }
        print_player_stats(game, 0);
        print_player_stats(game, 1);
        print_board_state(game);

        printf("PLAYER%d'S TURN\n", game->currentTurn + 1);
        printf("=== Choose a slot (1 to 12): ===\n");
        printf("Surrender (0):\n");
        
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
                if (surrender == 1){
                    // Change game state to over
                    return 1;
                } else if(surrender == 0){
                    break;
                }
            }

            if(slot >= 1){
                break;
            }
        }

        // printf("You selected slot %d.\n", slot);
    }

    return 0;
}
