//
// Created by Christopher Wilson on 08/11/2024.
//


#include "game.h"

int main() {
    Game game;
    load_game_state("game.json", &game);
    print_player_stats(&game, 0);
    save_game_state("game.json", &game);
    return 0;
}
