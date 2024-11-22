//
// Defines functions for the client
//

#ifndef AWALEGAME_CLIENT_H
#define AWALEGAME_CLIENT_H

#include <string.h>
#include "network.h"

// Attempt to log in. Return -1 if there is an error
int login(int server, char* username) {
    Request req = empty_request();
    req.action = LOGIN;
    strcpy(req.arguments[0], username);

    if (send_request(server, &req)) {
        fprintf(stderr, "Error: Could not send request to login.\n");
    }

    char* res = read_response(server);
    if (res == NULL) {
        fprintf(stderr, "Error: Could not read response from login request.\n");
        return -1;
    }

    if (strcmp(res, "true") != 0) {
        fprintf(stderr, "Server Error: Could not login.\n");
        free(res);
        return -1;
    }

    free(res);
    return 0;
}

// Attempt to show active users. Return -1 if there is an error
int list(int server) {
    Request req = empty_request();
    req.action = LIST;
    if (send_request(server, &req)) {
        fprintf(stderr, "Error: Could not send request.\n");
        return -1;
    }

    char* res = read_response(server);
    if (res == NULL) {
        fprintf(stderr, "Error: Could not retrieve list of online users.\n");
        return -1;
    }

    printf("Online users: %s\n", res);
    free(res);
    return 0;
}

int help() {
    printf("There are several actions you can perform once you have logged in.\n");
    printf("You can perform these actions whenever the prompt starts with 'AWALE //:'\n");
    printf("\u2022 help      - show this panel again\n");
    printf("\u2022 list      - list all currently online players\n");
    printf("\u2022 challenge - challenge a player\n");
    printf("\u2022 resume    - start or resume a game\n");
}

#endif //AWALEGAME_CLIENT_H
