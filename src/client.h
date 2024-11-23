//
// Defines functions for the client
//

#ifndef AWALEGAME_CLIENT_H
#define AWALEGAME_CLIENT_H

#include <string.h>
#include "network.h"

// Function to parse a JSON string into a dynamically allocated list of strings. Returned value needs to be freed
char** convert_name_string_to_list(const char *jsonString, int *item_count) {
    *item_count = 0;

    // Parse the JSON string
    cJSON *jsonArray = cJSON_Parse(jsonString);
    if (!jsonArray || !cJSON_IsArray(jsonArray)) {
        fprintf(stderr, "Error: Invalid JSON string or not an array\n");
        if (jsonArray) cJSON_Delete(jsonArray);  // Clean up if partially allocated
        return NULL;
    }

    // Get the number of items in the array
    int arraySize = cJSON_GetArraySize(jsonArray);

    // Dynamically allocate memory for an array of char* (string pointers)
    char** output = (char**) malloc(arraySize * sizeof(char*));
    if (!output) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        cJSON_Delete(jsonArray);
        return NULL;
    }

    // Loop through the JSON array and extract strings
    for (int i = 0; i < arraySize; i++) {
        cJSON *item = cJSON_GetArrayItem(jsonArray, i);
        if (cJSON_IsString(item)) {
            // Allocate memory for the string and copy it
            output[*item_count] = strdup(item->valuestring);
            if (!output[*item_count]) {
                fprintf(stderr, "Error: Memory allocation failed for string at index %d\n", i);
                for (int j = 0; j < *item_count; j++) {
                    free(output[j]);  // Free previously allocated strings
                }
                free(output);
                cJSON_Delete(jsonArray);
                return NULL;
            }
            (*item_count)++;  // Increment count of valid items
        } else {
            fprintf(stderr, "Warning: Non-string item at index %d\n", i);
        }
    }

    // Clean up
    cJSON_Delete(jsonArray);
    return output;
}

char** get_active_users(int server, char* username, int* no_users) {
    Request req = empty_request();
    req.action = LIST;
    strcpy(req.arguments[0], username);

    if (send_request(server, &req)) {
        fprintf(stderr, "Error: Could not send request.\n");
        return NULL;
    }

    char* res = read_response(server);
    if (res == NULL) {
        fprintf(stderr, "Error: Could not retrieve list of online users.\n");
        return NULL;
    }

    char** res1 = convert_name_string_to_list(res, no_users);
    free(res);

    if (res1 == NULL) {
        return NULL;
    }
    return res1;
}

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
int list(int server, char* username) {
    int len_users;
    char** users = get_active_users(server, username, &len_users);
    if (users == NULL) {
        return -1;
    }

    printf("Online users:\n");
    for (int i = 0; i < len_users; i++) {
        printf("\u2022 %s\n", users[i]);
    }

    free(users);
    return 0;
}

int challenge(int server, char* username) {
    // 1. Show online active_users to choose from
    int len_users;
    char** active_users = get_active_users(server, username, &len_users);
    if (active_users == NULL) {
        return -1;
    }
    printf("You can select a player to challenge from the below active users:\n");
    for (int i = 0; i < len_users; i++) {
        printf("%d. %s\n", i+1, active_users[i]);
    }
    printf("Select player number (1-%d)\n", len_users);

    // 2. Allow user to select a user
    printf("CHALLENGE // Enter player name: ");

    int choice;
    char input[BUFFER_SIZE];
    while (true) {
        if (scanf("%s", input) == -1) {
            perror("Error on input\n");
            continue;  // Skip the rest of the loop
        }

        choice = convert_and_validate(input, 1, len_users);

        if (choice < 0) {
            // Clear the invalid input from stdin buffer
            while (getchar() != '\n');
            printf("Invalid selection, try again.\n");
            continue;  // Skip the rest of the loop
        }

        break;
    }

    char chosen_user[MAX_NAME_LENGTH];
    strcpy(chosen_user, active_users[choice - 1]);
    free(active_users);

    printf("Challenging %s to a game.\n", chosen_user);

    Request req = empty_request();
    req.action = CHALLENGE;
    strcpy(req.arguments[0], username);
    strcpy(req.arguments[0], chosen_user);

    // 3. Send request
    if (send_request(server, &req)) {
        fprintf(stderr, "Error: Could not send request.\n");
        return -1;
    }

    // 4. Read response (true or false)
    char* res = read_response(server);
    if (res == NULL) {
        fprintf(stderr, "Error: Could not retrieve list of online active_users.\n");
        return -1;
    }
    if (strcmp(res, "false") == 0) {
        fprintf(stderr, "Error: Could not challenge user.\n");
        free(res);
        return -1;
    }
    if (strcmp(res, "true") == 0) {
        printf("Challenged user: %s\n", chosen_user);
        free(res);
        return 0;
    }
    fprintf(stderr, "Error: Unexpected response from server: %s\n", res);
    free(res);
    return 0;
}

// TODO: Respond to a challenge request. Return -1 if there is an error
int respond(int server, char* username) {
    printf("Not yet implemented\n");
    // 1. Display all requests to be either accepted or declined
    // 2. Pick a request from the list (in same way as in Challenge function)
    // 3. Either accept or decline
    // 4. If accepted, take user to play
    return 0;
}

// TODO: Resume a game already established between two users. Return -1 if there is an error
int play(int server, char* username) {
    printf("Not yet implemented\n");
    // 1. Display all the user's games
    // 2. Pick a game from the list (in same way as in Challenge function)
    // 3. play_game()
    return 0;
}

void help() {
    printf("There are several actions you can perform once you have logged in.\n");
    printf("You can perform these actions whenever the prompt starts with 'AWALE //:'\n");
    printf("\u2022 help      - show this panel again\n");
    printf("\u2022 list      - list all currently online players\n");
    printf("\u2022 challenge - challenge a player to a game of Awalé\n");
    printf("\u2022 respond   - accept/decline challenges received from other players\n");
    printf("\u2022 play      - start or resume a game\n");
    printf("\u2022 quit      - log out and quit\n");
}

#endif //AWALEGAME_CLIENT_H
