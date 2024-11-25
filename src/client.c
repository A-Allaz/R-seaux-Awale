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

int main(int argc, char** argv) {
    if (argc != 3) {
        printf("Usage: socket_client server port\n");
        exit(0);
    }

    // Initialise parameters
    struct sockaddr_in serv_addr;
    bzero((char*) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family       = AF_INET;
    serv_addr.sin_addr.s_addr  = inet_addr(argv[1]);
    serv_addr.sin_port         = htons(atoi(argv[2]));

    printf("Client starting...\n");

    // Create the socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Could not open socket\n");
        exit(EXIT_FAILURE);
    }

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

    printf("Welcome to:\n");
    printf("░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░\n░░      ░░░  ░░░░  ░░░      ░░░  ░░░░░░░░        ░\n▒  ▒▒▒▒  ▒▒  ▒  ▒  ▒▒  ▒▒▒▒  ▒▒  ▒▒▒▒▒▒▒▒  ▒▒▒▒▒▒▒\n▓  ▓▓▓▓  ▓▓        ▓▓  ▓▓▓▓  ▓▓  ▓▓▓▓▓▓▓▓      ▓▓▓\n█        ██   ██   ██        ██  ████████  ███████\n█  ████  ██  ████  ██  ████  ██        ██        █\n██████████████████████████████████████████████████\n");

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

        if (strcmp(input, "respond") == 0) {
            respond(server_socket, username);
            continue;
        }

        if (strcmp(input, "play") == 0) {
            play(server_socket, username);
            continue;
        }

        if (strcmp(input, "quit") == 0) {
            close(server_socket);
            printf("Logged out\n");
            break;
        }

        printf("Invalid command, please try again.\n");
        printf("(Enter help to see list of commands)\n");
    }

    return 0;
}