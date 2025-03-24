/*
 * Echo serveris
 * 
 * Author: Kęstutis Mizara
 * Description: Gauna kliento pranešimą ir išsiunčia atgal
 */

#ifdef _WIN32
#include <winsock2.h>
#define socklen_t int
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#ifndef _WIN32
#include <poll.h>
#include <stddef.h> // Include this header for nfds_t definition
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#define MAX_CONNECTED_CLIENTS 10
#define MAX_USERNAME_LENGTH 15

struct {
    int x;
    int y;
    char symbol;
} DrawPoint;

char board[20][20];

int draw(int x, int y, char symbol)
{
    if (x < 0 || x >= 20 || y < 0 || y >= 20)
    {
        return -1;
    }
    board[x][y] = symbol;
    return 0;
}

char* showBoard() {
    int width = 20; // Let's start with a smaller board
    int height = 20;
    char *strboard = (char*)malloc((width * (height + 1) + 1) * sizeof(char)); // Allocate memory
    if (strboard == NULL) {
        perror("Failed to allocate memory for board string");
        return NULL;
    }
    int index = 0;
    for (int i = 0; i < height; i++) {
        for (int j = 0; j < width; j++) {
            strboard[index++] = board[i][j] == 0 ? ' ' : board[i][j]; // Use space for empty cells
        }
        strboard[index++] = '\n';
    }
    strboard[index] = '\0'; // Null-terminate the string
    return strboard;
}


void sendBoardToClients(int current_client_fd, struct pollfd *pfds) {
    char* board_string = showBoard();
    if (board_string != NULL) {
        for (int i = 1; i <= MAX_CONNECTED_CLIENTS; i++) {
            if (pfds[i].fd != -1 && pfds[i].fd != current_client_fd) {
                send(pfds[i].fd, board_string, strlen(board_string), 0);
            }
        }
        free(board_string); // Free the allocated memory
    }
}




void resetBoard () {
    memset(board, 0, sizeof(board));
}

void commandParse (char *command, int client_fd, struct pollfd *pfds) {
    char *token = strtok(command, " ");
    if (strcmp(token, "/draw") == 0) {
        token = strtok(NULL, " ");
        if (token != NULL) {
            DrawPoint.x = atoi(token);
            token = strtok(NULL, " ");
            if (token != NULL) {
                DrawPoint.y = atoi(token);
                token = strtok(NULL, " ");
                if (token != NULL) {
                    if (strlen(token) == 1) {
                        DrawPoint.symbol = token[0];
                        int check = draw(DrawPoint.x, DrawPoint.y, DrawPoint.symbol);
                        if (check == -1) {
                            send(client_fd, "Invalid coordinates.\n", 21, 0);
                        } else {
                            sendBoardToClients(-1, pfds);
                            send(client_fd, "Draw successful.\n", 18, 0);
                        }
                    } else {
                        send(client_fd, "Invalid symbol. Please use a single character.\n", 46, 0);
                    }
                } else {
                    send(client_fd, "Usage: /draw <x> <y> <symbol>\n", 33, 0);
                }
            } else {
                send(client_fd, "Usage: /draw <x> <y> <symbol>\n", 33, 0);
            }
        } else {
            send(client_fd, "Usage: /draw <x> <y> <symbol>\n", 33, 0);
        }
    } else if (strcmp(token, "/show") == 0) {
        char* board_string = showBoard();
        if (board_string != NULL) {
            send(client_fd, board_string, strlen(board_string), 0);
            free(board_string);
        }
    } else if (strcmp(token, "/reset") == 0) {
        resetBoard();
        send(client_fd, "Board reset.\n", 13, 0);
        sendBoardToClients(-1, pfds);
    } else if (strcmp(token, "/help") == 0) {
        send(client_fd, "Available commands:\n/draw <x> <y> <symbol>\n/show\n/reset\n/help\n/exit\n", 64, 0);
    } else {
        send(client_fd, "Unknown command. Type /help for a list of available commands.\n", 64, 0);
    }
}






int main(int argc, char *argv []){
#ifdef _WIN32
    WSADATA data;
#endif
    unsigned int port;
    int l_socket; // socket'as skirtas prisijungimų laukimui
    int c_socket; // prisijungusio kliento socket'as

    struct sockaddr_in servaddr; // Serverio adreso struktūra
    struct sockaddr_in clientaddr; // Prisijungusio kliento adreso struktūra
    socklen_t clientaddrlen = sizeof(struct sockaddr);

    int s_len;
    int r_len;
    char buffer[1024];
    
    if (argc != 2){
        printf("USAGE: %s <port>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);

    if ((port < 1) || (port > 65535)){
        printf("ERROR #1: invalid port specified.\n");
        exit(1);
    }

    struct pollfd pfds[MAX_CONNECTED_CLIENTS + 1];
    char usernames[MAX_CONNECTED_CLIENTS + 1][MAX_USERNAME_LENGTH];
    memset(usernames, 0, sizeof(usernames));
    nfds_t nfds = MAX_CONNECTED_CLIENTS;

#ifdef _WIN32
    WSAStartup(MAKEWORD(2,2),&data);    
#endif

    /*
      * Sukuriamas serverio socket'as
      */
    if ((l_socket = socket(AF_INET, SOCK_STREAM,0))< 0){
        fprintf(stderr,"ERROR #2: cannot create listening socket.\n");
        exit(1);
    }
    
    /*
      * Išvaloma ir užpildoma serverio adreso struktūra
      */
    memset(&servaddr,0, sizeof(servaddr));
     servaddr.sin_family = AF_INET; // nurodomas protokolas (IP)

    /*
      * Nurodomas IP adresas, kuriuo bus laukiama klientų, šiuo atveju visi 
      * esami sistemos IP adresai (visi interfeis'ai)
      */
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
     servaddr.sin_port = htons(port); // nurodomas portas
    
    /*
      * Serverio adresas susiejamas su socket'u
      */
     if (bind (l_socket, (struct sockaddr *)&servaddr,sizeof(servaddr))<0){
        fprintf(stderr,"ERROR #3: bind listening socket.\n");
        exit(1);
    }

    /*
      * Nurodoma, kad socket'u l_socket bus laukiama klientų prisijungimo,
      * eilėje ne daugiau kaip 5 aptarnavimo laukiantys klientai
      */
    if (listen(l_socket, 5) <0){
        fprintf(stderr,"ERROR #4: error in listen().\n");
        exit(1);
    }

    pfds[0].fd = l_socket;
    pfds[0].events = POLLIN;

    for (int i = 1; i <= MAX_CONNECTED_CLIENTS; i++)
    {
        pfds[i].fd = -1;
    }

    for(;;){
            printf("Polling...\n");
            int activity = poll(pfds, nfds, -1);
            if (activity < 0)
            {
                fprintf(stderr, "poll error");
                exit(1);
            }
    
            if (pfds[0].revents & POLLIN)
            {
                printf("Trying to connect client.\n");
                if ((c_socket = accept(l_socket, (struct sockaddr *)&clientaddr,
                                        &clientaddrlen)) < 0)
                {
                    fprintf(stderr,
                            "ERROR #5: error occured accepting connection.\n");
                    exit(1);
                }
    
                for (int i = 1; i <= MAX_CONNECTED_CLIENTS; i++)
                {
                    if (pfds[i].fd == -1)
                    {
                        pfds[i].fd = c_socket;
                        pfds[i].events = POLLIN;
                        printf("Client %d fd: %d.\n", i, pfds[i].fd);
                        break;
                    }
                }
                printf("Client connected.\n");
            }
    
            for (int i = 1; i <= MAX_CONNECTED_CLIENTS; i++)
            {
                if (pfds[i].fd == -1)
                    continue;
    
                if (pfds[i].revents & POLLIN)
                {
                    if (usernames[i][0] == '\0')
                    {
                        send(pfds[i].fd, "Enter your username: ", 21, 0);
                        s_len =
                            recv(pfds[i].fd, usernames[i], sizeof(usernames[i]), 0);
                        if (s_len <= 0)
                        {
                            printf("Client %s disconnected.\n", usernames[i]);
                            close(pfds[i].fd);
                            usernames[i][0] = '\0';
                            pfds[i].fd = -1;
                        }
                        else
                        {
                            usernames[i][s_len] = '\0';
                            for (int j = 0; j < s_len; j++)
                            {
                                if (usernames[i][j] == '\n')
                                {
                                    usernames[i][j] = '\0';
                                }
                            }
                            printf("Client %d is now called %s.\n", i,
                                    usernames[i]);
                            
                            char str[MAX_USERNAME_LENGTH + 20];
                            sprintf(str, "%s connected.", usernames[i]);
                            send(pfds[i].fd, "Welcome to the server!\n", 23, 0);
                            for (int j = 1; j <= MAX_CONNECTED_CLIENTS; j++)
                            {
                                if (pfds[j].fd != -1 && i != j)
                                {
                                    send(pfds[j].fd, str, strlen(str), 0);
                                }
                            }
                        }
                    }
                    else
                    {
                    printf("Trying to read from client %d (%d).\n", i, pfds[i].fd);
                    size_t uname_length = strlen(usernames[i]);
                    strcpy(buffer, usernames[i]);
                    strcpy(buffer + uname_length, ": ");
                    s_len = recv(pfds[i].fd, buffer + (uname_length + 2),
                    sizeof(buffer) - (uname_length + 2), 0);
                    buffer[s_len + (uname_length + 2)] = '\0';

                    if (s_len <= 0)
                    {
                        char str[MAX_USERNAME_LENGTH + 20];
                        char *client_name =
                            usernames[i][0] == '\0' ? "Anon" : usernames[i];
                        sprintf(str, "%s disconnected.", client_name);
                        for (int j = 1; j <= MAX_CONNECTED_CLIENTS; j++)
                        {
                            if (pfds[j].fd != -1 && i != j)
                            {
                                r_len = send(pfds[j].fd, str, strlen(str), 0);
                            }
                        }
                        printf("Client disconnected.\n");
                        close(pfds[i].fd);
                        usernames[i][0] = '\0';
                        pfds[i].fd = -1;
                    }
                    else
                    {
                        printf("Received from client %d: \"%s\".\n", i, buffer);
                        if (buffer[uname_length + 2] == '/')
                        {
                            printf("Command detected: \"%s\"\n", buffer + uname_length + 2);
                            commandParse(buffer + uname_length + 2, pfds[i].fd, pfds);
                            // sendBoardToClients(pfds[i].fd, pfds);
                        }
                        else
                        {
                            printf("Broadcasting message to all clients.\n");
                            for (int j = 1; j <= MAX_CONNECTED_CLIENTS; j++)
                            {
                                if (pfds[j].fd != -1)
                                {
                                    r_len =
                                        send(pfds[j].fd, buffer, strlen(buffer), 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return 0;
}
