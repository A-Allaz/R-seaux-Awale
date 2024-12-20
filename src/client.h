//
// Defines functions for the client
//

#ifndef AWALEGAME_CLIENT_H
#define AWALEGAME_CLIENT_H

#include <string.h>
#include "network.h"

// Parse a JSON string into a dynamically allocated list of strings. Returned value needs to be freed
char** convert_name_string_to_list(const char* jsonString, int* item_count) {
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

// Perform request to get list of active users (with respect to a user)
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

// Perform request to get a list of a user's current games
char** get_current_games(int server, char* username, int* no_users) {
    Request req = empty_request();
    req.action = LIST_GAMES;
    strcpy(req.arguments[0], username);

    if (send_request(server, &req)) {
        fprintf(stderr, "Error: Could not send request.\n");
        return NULL;
    }

    char* res = read_response(server);
    if (res == NULL) {
        fprintf(stderr, "Error: Could not retrieve list of user's games.\n");
        return NULL;
    }

    char** res1 = convert_name_string_to_list(res, no_users);
    free(res);

    if (res1 == NULL) {
        return NULL;
    }
    return res1;
}


/**
 * @brief Manages the game loop between the current user and the chosen opponent.
 *
 * @param server The server socket.
 * @param username The username of the current player.
 * @param chosen_user The username of the opponent.
 *
 * @return int Returns 0 upon successful completion of the game loop, or -1 if an error occurs.
 *
 * @note The game state is updated in a loop by polling the server every 2 seconds until a change is detected.
 *  This times out after 10 refreshes.
 *
 */
int play_game(int server, char* username, char* chosen_user) {
    printf("Resuming game against %s.\n", chosen_user);

    // 1. Load current game state
    printf("Loading game...\n");
    Request req = empty_request();
    req.action = GAME;
    strcpy(req.arguments[0], username);
    strcpy(req.arguments[1], chosen_user);

    if (send_request(server, &req)) {
        fprintf(stderr, "Error: Could not send request.\n");
        return -1;
    }

    Game game;
    if (receive_game(server, &game)) {
        return -1;
    }

    // Check whether current user is player 0 or 1
    bool is_player0 = false;
    if (strcmp(username, game.player0) == 0) {
        is_player0 = true;
    }

    // 2. Await user's action (loops until game ends or user enters 'back')
    char input[BUFFER_SIZE];
    while (true) {
        clrscr();

        // Print stats out to user
        print_player_stats(&game, 0);
        print_player_stats(&game, 1);
        print_board_state(&game);

        // Other player's turn
        if ((game.current_state == MOVE_PLAYER_0 && ! is_player0) ||
                   (game.current_state == MOVE_PLAYER_1 && is_player0)) {
            printf("Waiting for other player to move\n");

            // Loop every second to update game state to check for changes
            GAME_STATE current_state = game.current_state;
            int retry_count = 0;
            const int max_retries = 100; // Allow 10 retries

            while (current_state == game.current_state) {
                if (retry_count++ >= max_retries) {
                    printf("Timed out waiting for the other player's move.\n");
                    return -1;
                }

                // Send request to server
                Request req1 = empty_request();
                req1.action = GAME;
                strcpy(req1.arguments[0], username);
                strcpy(req1.arguments[1], chosen_user);

                if (send_request(server, &req1)) {
                    fprintf(stderr, "Error: Could not send request.\n");
                    return -1;
                }

                if (receive_game(server, &game)) {
                    return -1;
                }

                sleep(2);
            }
            continue;
        }

        // Player has won
        if ((game.current_state == WIN_PLAYER_0 && is_player0) ||
            (game.current_state == WIN_PLAYER_1 && ! is_player0)) {
            printf("GAME OVER: You have won!\n");
            break;
        }

        // Player has not won
        if (game.current_state == WIN_PLAYER_1 && is_player0) {
            printf("GAME OVER: %s has won\n", game.player1);
            break;
        }
        if (game.current_state == WIN_PLAYER_0 && ! is_player0) {
            printf("GAME OVER: %s has won!\n", game.player0);
            break;
        }

        // Current player's turn
        if ((game.current_state == MOVE_PLAYER_0 && is_player0) ||
            (game.current_state == MOVE_PLAYER_1 && ! is_player0)) {
            printf("Choose a square (%d-%d) to empty seeds and distribute\n", (is_player0 + 1) % 2 * 6 + 1, (is_player0 + 1) % 2 * 6 + 6);
            printf("(enter 'back' to go home) (enter 0 to surrender)\n");
            printf("GAME //: ");
        }

        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            return 1;
        }

        // Check if there is a trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
        // while (getchar() != '\n' && getchar() != EOF);  // Clear the invalid input from stdin buffer
            input[len - 1] = '\0';  // Remove newline
        }

        // User wants to return home
        if (strcmp(input, "back") == 0) {
            break;
        }

        printf("Input: %s\n", input);
        // Get move from input
        int slot = convert_and_validate(input, 0, 12);
        if (slot < 0 || (is_player0 && slot > 6) || (!is_player0 && (slot < 7 && slot != 0))) {
            printf("Invalid input! Please enter a number between %d and %d.\n", (is_player0 + 1) % 2 * 6 + 1, (is_player0 + 1) % 2 * 6 + 6);
            continue;  // skip
        }

        printf("%d\n", slot);
        req = empty_request();
        req.action = MOVE;
        strcpy(req.arguments[0], username);
        strcpy(req.arguments[1], chosen_user);
        sprintf(req.arguments[2], "%d", slot);

        printf("%s\n", req.arguments[2]);

        // Make move
        if (send_request(server, &req)) {
            fprintf(stderr, "Error: Could not send request.\n");
            return -1;
        }

        // Get back false / new board state
        if (receive_game(server, &game)) {
            return -1;
        }
    }

    return 0;
}


/**
 * @brief Attempts to log in a user by sending a login request to the server.
 *
 * @param server The server socket.
 * @param username The username of the user attempting to log in.
 *
 * @return int Returns 0 if login is successful, or -1 if an error occurs.
 *
 * @note The server is expected to respond with either "true" (successful login) or "false" (login failure).
 */
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


/**
 * @brief Retrieves and displays the list of active users from the server.
 *
 * @param server The server socket.
 * @param username The username of the current user.
 *
 * @return int Returns 0 on success, or -1 if an error occurs.
 *
 * @note Memory allocated for the list of active users (array of strings) is dynamically managed and must be freed.
 */
int list(int server, char* username) {
    int len_users;
    char** users = get_active_users(server, username, &len_users);
    if (users == NULL) {
        return -1;
    }
    if (len_users == 0) {
        printf("There are no users online right now.\n");
        free(users);
        return 0;
    }
    printf("Online users:\n");
    for (int i = 0; i < len_users; i++) {
        printf("\u2022 %s\n", users[i]);
    }

    free(users);
    return 0;
}


/**
 * @brief Allows the user to challenge another online player to a game.
 *
 * @param server The server socket.
 * @param username The username of the current player initiating the challenge.
 *
 * @return int Returns 0 if the challenge is successfully sent and acknowledged, or -1 if an error occurs.
 */
int challenge(int server, char* username) {
    // 1. Show online active_users to choose from
    int len_users;
    char** active_users = get_active_users(server, username, &len_users);
    if (active_users == NULL) {
        return -1;
    }
    if (len_users == 0) {
        printf("There are no users online right now to challenge.\n");
        free(active_users);
        return 0;
    }
    printf("You can select a player to challenge from the below active users:\n");
    for (int i = 0; i < len_users; i++) {
        printf("%d. %s\n", i+1, active_users[i]);
    }

    // 2. Allow user to select a user
    printf("CHALLENGE // Enter player number (1-%d): ", len_users);

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

    // 3. Send request
    Request req = empty_request();
    req.action = CHALLENGE;
    strcpy(req.arguments[0], username);
    strcpy(req.arguments[1], chosen_user);

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
    fprintf(stderr, "Not yet implemented\n");
    // 1. Display all requests to be either accepted or declined
    // 2. Pick a request from the list (in same way as in Challenge function)
    // 3. Either accept or decline
    // 4. If accepted, take user to play
    return 0;
}


/**
 * @brief Facilitates user interaction to select and play one of their current games.
 *
 * @param server The socket used for communication with the server.
 * @param username The username of the current player.
 *
 * @return int Returns 0 upon successful execution, or -1 if an error occurs.
 */
int play(int server, char* username) {
    // 1. Display all the user's games
    int len_games;
    char** users_games = get_current_games(server, username, &len_games);
    if (users_games == NULL) {
        return -1;
    }

    if (len_games == 0) {
        printf("You have no games. To start one, use the challenge action.\n");
        free(users_games);
        return 0;
    }

    printf("Your current games:\n");
    for (int i = 0; i < len_games; i++) {
        printf("%d. Me <--> %s\n", i+1, users_games[i]);
    }
    // 2. Allow user to pick a game from the list
    printf("GAME // Select game number (1-%d): ", len_games);

    int choice;
    char input[BUFFER_SIZE];
    while (true) {
        if (fgets(input, sizeof(input), stdin) == NULL) {
            printf("Error reading input.\n");
            return 1;
        }

        // Check if there is a trailing newline
        size_t len = strlen(input);
        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';  // Remove newline
        }

        choice = convert_and_validate(input, 1, len_games);

        if (choice < 0) {
            // Clear the invalid input from stdin buffer
            while (getchar() != '\n');
            printf("Invalid selection, try again.\n");
            continue;  // Skip the rest of the loop
        }

        break;
    }

    char chosen_user[MAX_NAME_LENGTH];
    strcpy(chosen_user, users_games[choice - 1]);
    free(users_games);

    // 3. play_game()
    play_game(server, username, chosen_user);
    return 0;
}

/**
 * @brief Print out the possible commands that a user can perform
 */
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
