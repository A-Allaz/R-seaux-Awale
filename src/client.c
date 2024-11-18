//
// Created by Christopher Wilson on 08/11/2024.
//

#include "utils.h"
#include "game.h"
#include "network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT_NO 3000
#define SERVER_IP "127.0.0.1"

int main(){
    char player_name[255];

    int client_socket;
    struct sockaddr_in serv_addr;

    printf("Client starting...\n");

    // Create the socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("Could not open socket");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_NO);

    // Convert the server IP address
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, PORT_NO);

    Request req;
    req.action = 0;  // LOGIN action
    strncpy(req.arguments[0], "LIST", 255);
    req.arguments[1][0] = '\0';
    req.arguments[2][0] = '\0';

    // Send the request
    if (send_request(client_socket, &req) == 0) {
        printf("Request sent successfully.\n");
    } else {
        printf("Failed to send request.\n");
    }

    Game *game = malloc(sizeof(Game));
    if (game == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE); // Terminate the program with a failure status
    }

    printf("Enter your name: ");
    if (fgets(player_name, sizeof(player_name), stdin)) {
        // Remove trailing newline character if present
        player_name[strcspn(player_name, "\n")] = '\0'; 
    }

    init_game(game, player_name, "Enemy");
    
    int slot;
    while(game->score.player0 < 25 && game->score.player1 < 25) {
        clrscr();

        if (game->current_state == WIN_PLAYER_0 || game->current_state == WIN_PLAYER_1) {
            printf("GAME OVER\n");
            if(game->current_state == WIN_PLAYER_0){
                printf("%s WINS\n", game->player0);
            } else {
                printf("%s WINS\n", game->player1);
            }
            return 1;
        }

        print_player_stats(game, 0);
        print_player_stats(game, 1);
        print_board_state(game);

        if((game->current_state == MOVE_PLAYER_0 && !strcmp(game->player0, player_name)) 
        || (game->current_state == MOVE_PLAYER_1 && !strcmp(game->player1, player_name))){
            printf("%s'S TURN\t", player_name);
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
                    if (surrender == 1){
                        game->current_state == WIN_PLAYER_1;
                        break;
                    } else if(surrender == 0){
                        break;
                    }
                }

                if(slot >= 1){
                    play_turn(game, slot - 1);
                    break;
                }
            }
        } else {
            printf("==== WAITING FOR %s TO PLAY ====\n", game->player1);
            printf("waiting...\r\n");
            sleep(3);
            return 0;
        }
    }

    return 0;

    return 0;
}