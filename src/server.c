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
