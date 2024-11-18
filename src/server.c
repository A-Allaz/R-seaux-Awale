//
// Created by Christopher Wilson on 08/11/2024.
//


#include "game.h"
#include "network.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>

#include "utils.h"

int interactive_play(Game *game);

int handle(int newsockfd);

int list(int newsockfd);

int make_move(Game *game, int slot);

int main() {
    const in_port_t PORT_NO = 3000;
    int server_socket, client_socket;
    socklen_t clilen;

    struct sockaddr_in cli_addr,serv_addr;

    printf ("Server starting...\n");

    // Open socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf ("Could not open socket\n");
        exit(0);
    }

    // Initialise parameters
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family       = AF_INET;
    serv_addr.sin_addr.s_addr  = htonl(INADDR_ANY);
    serv_addr.sin_port         = PORT_NO;

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("Error binding\n");
        close(server_socket);
        exit(errno);
    }

    // Begin listening
    if (listen(server_socket, 5) < 0) {
        printf("Error on listen\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", PORT_NO);

    // Wait for client to connect
    while (1) {
        clilen = sizeof (cli_addr);

        // Client connects, attempt to accept
        client_socket = accept(server_socket, (struct sockaddr*) &cli_addr, &clilen);

        if (client_socket < 0) {
            printf("Error accepting\n");
            exit(errno);
        }

        printf("Connection accepted\n");

        // Fork into thread to handle asynchronously
        int pid = fork();

        // Error case
        if (pid < 0) {
            perror("Error on fork");
            close(client_socket);
            continue;
        }

        // Child thread
        else if (pid == 0) {
            close(server_socket);
            handle(client_socket);
            close(client_socket);
            exit(0); /* Terminate child process */
        }

        // Main thread
        else {
            close(client_socket);
        }

    }

    // Close server socket at end
    close(server_socket);

    return 0;
}

int handle(int newsockfd) {
    Request req;
    if (receive_request(newsockfd, &req)) {
        perror("error getting request");
        return 1;
    }

    switch (req.action) {
        case LOGIN:
            break;
        case CHALLENGE:
            break;
        case ACCEPT:
            break;
        case LIST:
            list(newsockfd);
        case MOVE:
            break;
    }
}

// Return
int list(int newsockfd) {
    int count;
    char** players = get_online_players(&count);

    if (players == NULL) {
        fprintf(stderr, "Error: Could not retrieve online players.\n");
        return 1;
    }

    // Send the count of players first
    if (send(newsockfd, &count, sizeof(count), 0) == -1) {
        perror("Error sending player count");
        for (int i = 0; i < count; i++) {
            free(players[i]);
        }
        free(players);
        return 1;
    }

    // Send each player name
    for (int i = 0; i < count; i++) {
        if (send(newsockfd, players[i], 255, 0) == -1) {
            perror("Error sending player name");

            // Free allocated memory before returning
            for (int j = 0; j < count; j++) {
                free(players[j]);
            }
            free(players);
            return 1;
        }
    }

    // Free allocated memory
    for (int i = 0; i < count; i++) {
        free(players[i]);
    }
    free(players);

    return 0;
}

// Perform move given a slot (ASSUMED TO BE VALID)
int make_move(Game *game, int slot) {
    // Perform turn
    play_turn(game, slot - 1);

    // Save game state
    save_game_state("game.json", game);

    // Broadcast game to players
    broadcast_game_state();
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

        printf("PLAYER%d'S TURN\n", game->current_state + 1);
        printf("=== Choose a slot (1 to 12): ===\n");

        while (1) {
            // Attempt to read an integer
            if (scanf("%d", &slot) == 1 && slot >= 1 && slot <= 12) {
                break;  // Input is valid, break the loop
            } else {
                // Clear the invalid input from stdin buffer
                while (getchar() != '\n'); // Flush the input buffer

                // Display an error message
                printf("Invalid input! Please enter a number between 1 and 12.\n");

                // Prompt for input again
                printf("Choose a slot (1-12):\n");
            }
        }

        printf("You selected slot %d.\n", slot);
    }

    return 0;
}
