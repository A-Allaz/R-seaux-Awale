//
// Created by Christopher Wilson on 08/11/2024.
//

#ifndef AWALEGAME_REQUEST_H
#define AWALEGAME_REQUEST_H

#endif //AWALEGAME_REQUEST_H

typedef struct request {
    char action[11];  // action can be 10 chars long
    char* arguments;
} Request;