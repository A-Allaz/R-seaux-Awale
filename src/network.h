//
// Defines shared resources for communicating on the network.
// To be used by both client and server
//

#ifndef AWALEGAME_REQUEST_H

#define AWALEGAME_REQUEST_H

#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define PORT_NO 3000
#define MAX_ARG_LENGTH 255

// Initialise server given port no, and socket address. If error, halts program
int initialize_server(in_port_t port, struct sockaddr_in *serv_addr) {
    int server_socket;

    // Open socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Could not open socket");
        exit(EXIT_FAILURE);
    }

    // Initialize server address structure
    bzero((char *) serv_addr, sizeof(*serv_addr));
    serv_addr->sin_family = AF_INET;
    serv_addr->sin_addr.s_addr = INADDR_ANY;
    serv_addr->sin_port = htons(port);

    // Bind the socket to the address
    if (bind(server_socket, (struct sockaddr *) serv_addr, sizeof(*serv_addr)) < 0) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_socket, 5) < 0) {
        perror("Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server initialized and listening on port %d\n", port);
    return server_socket;
}

typedef enum {
    LOGIN,      // Log in
    LIST,       // List all active players (excluding current user)
    CHALLENGE,  // Challenge another player
    ACCEPT,     // Accept a challenge request
    DECLINE,    // Decline a challenge request
    GAME,       // Retrieve a game
    LIST_GAMES, // Retrieve all user's games
    MOVE        // Make a move within a game
} ACTION;

typedef struct {
    ACTION action;
    char arguments[3][MAX_ARG_LENGTH];
} Request;

Request empty_request() {
    Request req;
    req.action = LOGIN;
    strcpy(req.arguments[0], "");
    strcpy(req.arguments[1], "");
    strcpy(req.arguments[2], "");
    return req;
}

const char* action_to_string(ACTION action) {
    switch (action) {
        case LOGIN: return "LOGIN";
        case LIST: return "LIST";
        case CHALLENGE: return "CHALLENGE";
        case ACCEPT: return "ACCEPT";
        case DECLINE: return "DECLINE";
        case GAME: return "GAME";
        case LIST_GAMES: return "LIST_GAMES";
        case MOVE: return "MOVE";
        default: return NULL;
    }
}

int string_to_action(const char* action_str, ACTION* action) {
    if (strcmp(action_str, "LOGIN") == 0)
        *action = LOGIN;
    else if (strcmp(action_str, "LIST") == 0)
        *action = LIST;
    else if (strcmp(action_str, "CHALLENGE") == 0)
        *action = CHALLENGE;
    else if (strcmp(action_str, "ACCEPT") == 0)
        *action = ACCEPT;
    else if (strcmp(action_str, "DECLINE") == 0)
        *action = DECLINE;
    else if (strcmp(action_str, "GAME") == 0)
        *action = GAME;
    else if (strcmp(action_str, "LIST_GAMES") == 0)
        *action = LIST_GAMES;
    else if (strcmp(action_str, "MOVE") == 0)
        *action = MOVE;
    else
        return -1;
    return 0;
}

void print_request(const Request* request) {
    if (!request) {
        printf("Request is NULL\n");
        return;
    }

    printf("Request:\n");
    printf("  Action: %s\n", action_to_string(request->action));
    printf("  Arguments:\n");
    for (int i = 0; i < 3; i++) {
        printf("    [%d]: %s\n", i, request->arguments[i]);
    }
}

// Function to convert Request to JSON - returned value must be freed
char* request_to_json(const Request* request) {
    if (!request) {
        return NULL;  // Null check for the input
    }

    // Create a JSON object
    cJSON* json_request = cJSON_CreateObject();
    if (!json_request) {
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;  // Return NULL if memory allocation fails
    }

    const char* action = action_to_string(request->action);
    if (action == NULL) {
        fprintf(stderr, "Error: Invalid enum value given in request object\n");
        return NULL;
    }

    // Add the action as a string
    cJSON_AddStringToObject(json_request, "action", action_to_string(request->action));

    // Add the arguments as a JSON array
    cJSON* json_arguments = cJSON_CreateArray();
    for (int i = 0; i < 3; i++) {
        cJSON_AddItemToArray(json_arguments, cJSON_CreateString(request->arguments[i]));
    }
    cJSON_AddItemToObject(json_request, "arguments", json_arguments);

    // Convert the JSON object to a string
    char* json_string = cJSON_Print(json_request);

    // Free the JSON object (but keep the string)
    cJSON_Delete(json_request);

    return json_string;
}

// Function to parse JSON into a Request struct - returned value indicates error
int json_to_request(const char* json_string, Request* request) {
    if (!json_string || !request) {
        return -1;  // Invalid input
    }

    // Parse the JSON string
    cJSON* json_request = cJSON_Parse(json_string);
    if (!json_request) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        return -1;
    }

    // Extract the action field
    cJSON* action_item = cJSON_GetObjectItemCaseSensitive(json_request, "action");
    if (!cJSON_IsString(action_item) || !action_item->valuestring) {
        fprintf(stderr, "Error: 'action' field is missing or invalid\n");
        cJSON_Delete(json_request);
        return -1;
    }

    if (string_to_action(action_item->valuestring, &request->action)) {
        fprintf(stderr, "Error: 'action' field does not match the ACTION enum\n");
        cJSON_Delete(json_request);
        return -1;
    }

    // Extract the arguments field
    cJSON* arguments_array = cJSON_GetObjectItemCaseSensitive(json_request, "arguments");
    if (!cJSON_IsArray(arguments_array)) {
        fprintf(stderr, "Error: 'arguments' field is missing or invalid\n");
        cJSON_Delete(json_request);
        return -1;
    }

    // Fill the arguments array in the struct
    for (int i = 0; i < 3; i++) {
        cJSON* argument_item = cJSON_GetArrayItem(arguments_array, i);
        if (cJSON_IsString(argument_item) && argument_item->valuestring) {
            strncpy(request->arguments[i], argument_item->valuestring, MAX_ARG_LENGTH);
        } else {
            request->arguments[i][0] = '\0';  // Empty string for missing/invalid items
        }
    }

    // Free the JSON object
    cJSON_Delete(json_request);

    return 0;  // Success
}

// Receive a JSON string from a socket
int receive_request(int socket_from, Request* req) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Receive data from the socket_from
    int bytes_received = recv(socket_from, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received < 0) {
        perror("Error receiving data");
        return -1;
    } else if (bytes_received == 0) {
        printf("Connection closed by peer\n");
        return -1;
    }

    // Allocate memory for the received JSON string
    char *json_string = (char *) malloc(bytes_received + 1);
    if (!json_string) {
        fprintf(stderr, "Memory allocation failed\n");
        return -1;
    }

    // Copy the received data into the allocated memory and null-terminate it
    strncpy(json_string, buffer, bytes_received);
    json_string[bytes_received] = '\0';

//    printf("JSON Received: %s\n", json_string);

    // Form request object from JSON string
    if (json_to_request(json_string, req)) {
        perror("Error forming Request object from retrieved JSON\n");
        free(json_string);
        return -1;
    }

    free(json_string);
    return 0;
}

// Send a Request to a socket
int send_request(int socket_to, const Request* req) {
//    printf("Sending %s request\n", action_to_string(req->action));
    char* json = request_to_json(req);

    if (json == NULL) {
        perror("Error converting to json\n");
        return -1;
    }

//    printf("JSON to send: %s\n", json);

    // Send the serialized request over the socket_to
    ssize_t bytes_sent = send(socket_to, json, strlen(json), 0);
    free(json);  // Free the json string regardless of success or failure
    if (bytes_sent < 0) {
        perror("Error sending data\n");
        return -1;
    }
    return 0;
}

// Read response (intended to be used to read responses from server) - returned value must be freed
char* read_response(int socket) {
    char buffer[BUFFER_SIZE];  // Temporary buffer for reading
    char *result = NULL;       // Pointer to hold the full string
    size_t result_size = 0;    // Size of the dynamically allocated result buffer
    ssize_t bytes_received;

    while ((bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';  // Null-terminate the chunk

        // Allocate or expand memory for the result buffer
        char *new_result = realloc(result, result_size + bytes_received + 1);
        if (!new_result) {
            perror("realloc failed\n");
            free(result);
            return NULL;
        }
        result = new_result;

        // Append the received chunk to the result buffer
        memcpy(result + result_size, buffer, bytes_received + 1);
        result_size += bytes_received;

        // Optional: Stop reading if you detect a termination condition
        if (strchr(buffer, '\0')) {  // If a null terminator is received, stop
            break;
        }
    }

    if (bytes_received < 0) {
        perror("recv failed\n");
        free(result);
        return NULL;
    }

    return result;  // Caller is responsible for freeing the memory
}

// TODO: Read game object from socket
int receive_game(int socket, Game* game) {
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);

    // Receive the JSON string from the server
    int bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received <= 0) {
        fprintf(stderr, "Error: Failed to receive game state\n");
        return -1;
    }

    // Null-terminate the received string
    buffer[bytes_received] = '\0';

    // Parse the JSON string into the Game structure
    cJSON* game_json = cJSON_Parse(buffer);
    if (game_json == NULL) {
        fprintf(stderr, "Error: Failed to parse JSON\n");
        return -1;
    }

    // Extract player0 and player1
    cJSON* player0_json = cJSON_GetObjectItem(game_json, "player0");
    cJSON* player1_json = cJSON_GetObjectItem(game_json, "player1");
    if (player0_json && player1_json) {
        strncpy(game->player0, player0_json->valuestring, MAX_NAME_LENGTH);
        strncpy(game->player1, player1_json->valuestring, MAX_NAME_LENGTH);
    }

    // Extract current game state
    cJSON* state_json = cJSON_GetObjectItem(game_json, "currentState");
    if (state_json) {
        game->current_state = (GAME_STATE)state_json->valueint;
    }

    // Extract the score
    cJSON* score_json = cJSON_GetObjectItem(game_json, "score");
    if (score_json) {
        cJSON* player0_score_json = cJSON_GetObjectItem(score_json, "player0");
        cJSON* player1_score_json = cJSON_GetObjectItem(score_json, "player1");
        if (player0_score_json && player1_score_json) {
            game->score.player0 = player0_score_json->valueint;
            game->score.player1 = player1_score_json->valueint;
        }
    }

    // Extract the board
    cJSON* board_json = cJSON_GetObjectItem(game_json, "board");
    if (board_json && cJSON_IsArray(board_json)) {
        int board_size = cJSON_GetArraySize(board_json);
        for (int i = 0; i < board_size && i < BOARD_SIZE; i++) {
            cJSON* cell_json = cJSON_GetArrayItem(board_json, i);
            if (cell_json) {
                game->board[i] = cell_json->valueint;
            }
        }
    }

    // Clean up
    cJSON_Delete(game_json);
    return 0;
}

#endif
