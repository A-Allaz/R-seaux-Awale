//
// Created by Christopher Wilson on 08/11/2024.
//


#include "game.h"

int main() {
    Game *game = malloc(sizeof(Game));
    initGame(game,"Adam","Chris");
    print_board_state(game);

    // load_game_state("game.json", game);
    // print_player_stats(game, 0);
    // print_player_stats(game, 1);
    // save_game_state("game.json", game);
    return 0;
}

int interactivePlay(Game *game) {
    while(game->score.player0 < 25 || game->score.player1 < 25) {
        printf("player%d turn :", game->currentTurn);
    }
}
