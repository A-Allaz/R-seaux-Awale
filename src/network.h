//
// Created by Christopher Wilson on 08/11/2024.
//

#include <sys/socket.h>

#ifndef AWALEGAME_REQUEST_H
#define AWALEGAME_REQUEST_H

#endif //AWALEGAME_REQUEST_H

typedef enum {
    LOGIN,
    CHALLENGE,
    ACCEPT,
    LIST,
    MOVE
} ACTION;

typedef struct {
    ACTION action;
    char arguments[3][255];
} Request;

// Function to parse the incoming message and populate the Request structure
int parse_request(char *buffer, Request *req) {
    // Tokenize the buffer using "?" as the delimiter
    char *token = strtok(buffer, "?");

    if (token == NULL) {
        printf("Invalid format");
        return -1;  // Invalid request format
    }

    // Parse the action
    if (strcmp(token, "LOGIN") == 0) {
        req->action = LOGIN;
    } else if (strcmp(token, "CHALLENGE") == 0) {
        req->action = CHALLENGE;
    } else if (strcmp(token, "ACCEPT") == 0) {
        req->action = ACCEPT;
    } else if (strcmp(token, "LIST") == 0) {
        req->action = LIST;
    } else if (strcmp(token, "MOVE") == 0) {
        req->action = MOVE;
    } else {
        printf("Unknown action given");
        return -1;  // Unknown action
    }

    // Initialize arguments array and count
    int n = 0;

    // Parse the arguments
    while ((token = strtok(NULL, "?")) != NULL) {
        if (n > 3) {
            printf("Too many arguments given");
            return -1;
        }
        strncpy(req->arguments[n], token, 255);
        n++;
    }

    return 0;
}

// Function to handle receiving data from the socket and parsing it
int receive_request(int sockfd, Request *req) {
    char buffer[1024];
    ssize_t bytes_received = recv(sockfd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_received < 0) {
        perror("Error receiving data");
        return -1;
    }

    buffer[bytes_received] = '\0';  // Null-terminate the received data

    // Parse the request from the buffer
    return parse_request(buffer, req);
}


int broadcast_game_state() {

    return 0;
}