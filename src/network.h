#include <sys/socket.h>

#ifndef AWALEGAME_REQUEST_H
#define AWALEGAME_REQUEST_H
#define BUFFER_SIZE 1024  // Adjust based on your needs


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

const char* action_to_string(ACTION action) {
    switch (action) {
        case LOGIN: return "LOGIN";
        case CHALLENGE: return "CHALLENGE";
        case ACCEPT: return "ACCEPT";
        case LIST: return "LIST";
        case MOVE: return "MOVE";
        default: return NULL;
    }
}

int string_to_action(const char* action_str, ACTION* action) {
    if (strcmp(action_str, "LOGIN") == 0)
        *action = LOGIN;
    else if (strcmp(action_str, "CHALLENGE") == 0)
        *action = CHALLENGE;
    else if (strcmp(action_str, "ACCEPT") == 0)
        *action = ACCEPT;
    else if (strcmp(action_str, "LIST") == 0)
        *action = LIST;
    else if (strcmp(action_str, "MOVE") == 0)
        *action = MOVE;
    else
        return -1;
    return 0;
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
            strncpy(request->arguments[i], argument_item->valuestring, 255);
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

    printf("JSON Received: %s\n", json_string);

    // Form request object from JSON string
    if (json_to_request(json_string, req)) {
        perror("Error forming Request object from retrieved JSON\n");
        return -1;
    }

    free(json_string);
    return 0;
}

// Send a Request to a socket
int send_request(int socket_to, const Request* req) {
    char* json = request_to_json(req);

    if (json == NULL) {
        perror("Error converting to json\n");
        return -1;
    }

    printf("JSON to send: %s\n", json);

    // Send the serialized request over the socket_to
    ssize_t bytes_sent = send(socket_to, json, strlen(json), 0);
    free(json);  // Free the json string regardless of success or failure
    if (bytes_sent < 0) {
        perror("Error sending data");
        return -1;
    }
    return 0;
}

//// Function to parse the incoming message and populate the Request structure
//int parse_request(char *buffer, Request *req) {
//    // Tokenize the buffer using "?" as the delimiter
//    char *token = strtok(buffer, "?");
//
//    if (token == NULL) {
//        printf("Invalid format");
//        return -1;  // Invalid request format
//    }
//
//    // Parse the action
//    if (strcmp(token, "LOGIN") == 0) {
//        req->action = LOGIN;
//    } else if (strcmp(token, "CHALLENGE") == 0) {
//        req->action = CHALLENGE;
//    } else if (strcmp(token, "ACCEPT") == 0) {
//        req->action = ACCEPT;
//    } else if (strcmp(token, "LIST") == 0) {
//        req->action = LIST;
//    } else if (strcmp(token, "MOVE") == 0) {
//        req->action = MOVE;
//    } else {
//        printf("Unknown action given");
//        return -1;  // Unknown action
//    }
//
//    // Initialize arguments array and count
//    int n = 0;
//
//    // Parse the arguments
//    while ((token = strtok(NULL, "?")) != NULL) {
//        if (n > 3) {
//            printf("Too many arguments given");
//            return -1;
//        }
//        strncpy(req->arguments[n], token, 255);
//        n++;
//    }
//
//    return 0;
//}

#endif //AWALEGAME_REQUEST_H
