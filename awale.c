#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef struct Score {
    int player0;
    int player1;
} Score;

typedef struct Game {
    char* player0;
    char* player1; 
    int currentTurn;    // 0 or 1
    Score score;
    int* boardstate;    // 12 slots
    int status;         // 0 for playing and 1 for ended
} Game;

int initScore(Score score){
    score.player0 = 0;
    score.player1 = 0;
    return 0;
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

int playTurn(Game* game, int slot){
    int pebbles = movePebbles(game, slot);
    computeScore(game, slot, pebbles);

    // check victory : if returns 1, the current player won the game through points
    if(game->currentTurn == 0 && game->score.player0 > 24){
        return 1;
    } else if(game->currentTurn == 0 && game->score.player0 > 24){
        return 1;
    }

    // switch turn
    game->currentTurn = (game->currentTurn + 1) % 2;
    return 0;
}

int movePebbles(Game* game, int slot){ //
    int pebbles = game->boardstate[slot];
    for (int i = 0; i < pebbles;){
        if((slot + i) % 12 != slot){
            game->boardstate[(slot + i) % 12]++;
            i++;
        }
    }
    game->boardstate[slot] = 0;

    return pebbles;
}

int computeScore(Game* game, int slot, int pebbles){
    int i = 0;
    int current = game->currentTurn;

    int lastSlot = (slot + pebbles - i) % 12;
    int slotScore = game->boardstate[lastSlot];

    while(slotScore == 2 || slotScore == 3){
        if(current = 0 && lastSlot >= 6){ // in player1's side
            game->score.player0 += slotScore;
        } else if(current = 1 && lastSlot < 6){ // in player0's side
            game->score.player1 += slotScore;
        }

        i++;

        int lastSlot = (slot + pebbles - i) % 12;
        int slotScore = game->boardstate[lastSlot];
    }

    return 0;
}