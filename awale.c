#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct Score{
    int player0;
    int player1;
}Score;

typedef struct Game {
    char* player0;
    char* player1; 
    int currentTurn;    // 0 or 1
    Score score;
    int* boardstate;    // 12 slots
}Game;

int initScore(Score score){
    score.player0 = 0;
    score.player1 = 0;
}

Game* initGame(char* player0, char* player1){
    Game* game = malloc(sizeof(Game));
    game->player0 = player0;
    game->player1 = player1;
    game->currentTurn = 0;
    initScore(game->score);
    game->boardstate = malloc(12 * sizeof(int));
    for (int i = 0; i < 12; i++){
        game->boardstate[i] = 4;
    }
    return game;
}