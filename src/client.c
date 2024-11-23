#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "utils.h"
#include "game.h"
#include "network.h"
#include "client.h"

#define SERVER_IP "127.0.0.1"

int main() {
    char player_name[255];

    int server_socket;
    struct sockaddr_in serv_addr;

    printf("Client starting...\n");

    // Create the socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Could not open socket\n");
        exit(EXIT_FAILURE);
    }

    // Set up the server address struct
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT_NO);

    // Convert the server IP address
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(server_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection failed\n");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Connected to server at %s:%d\n", SERVER_IP, PORT_NO);

    printf("Welcome to Awalé\n");

    // 1. Log in
    char username[MAX_NAME_LENGTH + 2];  // +2 to account for '\n' and '\0'
    printf("LOGIN // Enter username (max %d characters): ", MAX_NAME_LENGTH);
    while (true) {
        if (fgets(username, sizeof(username), stdin) != NULL) {
            size_t len = strlen(username);

            if (len > MAX_NAME_LENGTH) {
                printf("Username too long. Please try again.\n");
                // Clear the buffer
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                continue;
            }

            // Check if there is a trailing newline
            if (len > 0 && username[len - 1] == '\n') {
                username[len - 1] = '\0';  // Remove newline
                len --;  // Update the length variable
            }

            // Check if input is empty
            if (len == 0) {
                printf("Username cannot be empty.\n");
                continue;
            }

            printf("Username accepted: %s\n", username);
            // Attempt log in
            if (login(server_socket, username)) {
                fprintf(stderr, "Error: Could not log in\n");
                continue;
            }
            break;
        } else {
            printf("Error reading input.\n");
            return 1;
        }
    }

    // 2. Show actions that user can do (help)
    help();

    // 3. Await user's action (loops until program ends or user enters 'quit')
    char input[BUFFER_SIZE];
    while (true) {
        printf("AWALE //: ");
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            return 1;
        }

        // Check if there is a trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';  // Remove newline
        }

        // Check what action was entered:
        if (strcmp(input, "help") == 0) {
            help();
            continue;
        }

        if (strcmp(input, "list") == 0) {
            list(server_socket, username);
            continue;
        }

        if (strcmp(input, "challenge") == 0) {
            challenge(server_socket, username);
            continue;
        }

        if (strcmp(input, "play") == 0) {
            resume(server_socket);
            continue;
        }

        if (strcmp(input, "quit") == 0) {
            close(server_socket);
            printf("Logged out");
            break;
        }

        printf("Invalid command, please try again.\n");
        printf("(Enter help to see list of commands)\n");
    }

    // Ideally the program should end here when the user enters quit or something
    return 0;

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
    
    while (game->score.player0 < 25 && game->score.player1 < 25) {
        clrscr();

        if (game->current_state == WIN_PLAYER_0 || game->current_state == WIN_PLAYER_1) {
            printf("GAME OVER\n");
            if (game->current_state == WIN_PLAYER_0) {
                printf("%s WINS\n", game->player0);
            } else {
                printf("%s WINS\n", game->player1);
            }
            free(game);
            return 1;
        }

        print_player_stats(game, 0);
        print_player_stats(game, 1);
        print_board_state(game);

        if ((game->current_state == MOVE_PLAYER_0 && !strcmp(game->player0, player_name)) ||
            (game->current_state == MOVE_PLAYER_1 && !strcmp(game->player1, player_name))) {
            printf("%s'S TURN\t", player_name);
            printf("\033[0;93m"); // Yellow
            printf("Surrender (0)\n");
            printf("\033[0;94m"); // Blue
            printf("=== Choose a slot (1 to 12): ===\n");
            printf("\033[0;39m"); // White

            while (1) {
                char input[4];  // input doesn't need to be bigger than 3 chars
                if (scanf("%3s", input) == -1) {
                    perror("Error on input\n");
                    continue;  // Skip the rest of the loop
                }

                int slot = convert_and_validate(input, 0, 12);
                if (slot < 0) {
                    // Clear the invalid input from stdin buffer
                    while (getchar() != '\n');
                    printf("Invalid input! Please enter a number between 0 and 12.\n");
                    continue;  // Skip the rest of the loop
                }

                // Case : surrender
                if (slot == 0) {
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
                        game->current_state = WIN_PLAYER_1;
                        break;
                    } else if (surrender == 0) {
                        break;
                    }
                }

                if (slot >= 1) {
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
}