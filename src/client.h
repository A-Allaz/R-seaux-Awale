//
// Defines functions for the client
//

#ifndef AWALEGAME_CLIENT_H
#define AWALEGAME_CLIENT_H

#include <string.h>
#include "network.h"

// Attempt to log in. Return -1 if there is an error
int login(int server, char* username) {
    Request req;
    req.action = LOGIN;
    strcpy(req.arguments[0], username);
    return send_request(server, &req);
}

// Attempt to show active users. Return -1 if there is an error
int list(int server) {
    Request req;
    req.action = LIST;
    if (send_request(server, &req)) {
        // handle error TODO
        return -1;
    }

    // get returned list and print TODO

    return 0;
}

int help() {
    printf("There are several actions you can perform once you have logged in.\n");
    printf("You can perform these actions whenever there is a '//' in the input prompt.\n");
    printf("\u2022 help      - show this panel again\n");
    printf("\u2022 list      - list all currently online players\n");
    printf("\u2022 challenge - challenge a player\n");
    printf("\u2022 resume    - start or resume a game\n");
}

#endif //AWALEGAME_CLIENT_H
