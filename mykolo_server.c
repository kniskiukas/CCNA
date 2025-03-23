#include <sys/poll.h>
#ifdef _WIN32
#include <winsock2.h>
#define socklen_t int
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#endif

#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_CONNECTED_CLIENTS 10
#define MAX_USERNAME_LENGTH 15

int main(int argc, char *argv[]) {
#ifdef _WIN32
    WSADATA data;
#endif
    unsigned int port;
    int l_socket; // socket'as skirtas prisijungim� laukimui
    int c_socket; // prisijungusio kliento socket'as

    struct sockaddr_in servaddr;   // Serverio adreso strukt�ra
    struct sockaddr_in clientaddr; // Prisijungusio kliento adreso strukt�ra
    socklen_t clientaddrlen = sizeof(struct sockaddr);

    int s_len;
    int r_len;
    char buffer[1024];

    if (argc != 2)
    {
        printf("Usage %s port\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    port = atoi(argv[1]);

    if ((port < 1) || (port > 65535))
    {
        fprintf(stderr, "ERROR #1: invalid port specified.\n");
        exit(EXIT_FAILURE);
    }

    struct pollfd pfds[MAX_CONNECTED_CLIENTS + 1];
    char usernames[MAX_CONNECTED_CLIENTS + 1][MAX_USERNAME_LENGTH];
    memset(usernames, 0, sizeof(usernames));
    nfds_t nfds = MAX_CONNECTED_CLIENTS;

#ifdef _WIN32
    WSAStartup(MAKEWORD(2, 2), &data);
#endif

    if ((l_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET; // nurodomas protokolas (IP)

    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port); // nurodomas portas

    if (bind(l_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        fprintf(stderr, "ERROR #3: bind listening socket.\n");
        exit(EXIT_FAILURE);
    }

    if (listen(l_socket, 5) < 0)
    {
        fprintf(stderr, "ERROR #4: error in listen().\n");
        exit(EXIT_FAILURE);
    }

    pfds[0].fd = l_socket;
    pfds[0].events = POLLIN;

    for (int i = 1; i <= MAX_CONNECTED_CLIENTS; i++)
    {
        pfds[i].fd = -1;
    }

    while (1)
    {
        printf("Polling...\n");
        int activity = poll(pfds, nfds, -1);
        if (activity < 0)
        {
            fprintf(stderr, "poll error");
            exit(EXIT_FAILURE);
        }
        printf("Activity: %d.\n", activity);

        // naujas klientas nori prisijungti
        if (pfds[0].revents & POLLIN)
        {
            printf("Trying to connect client.\n");
            if ((c_socket = accept(l_socket, (struct sockaddr *)&clientaddr,
                                   &clientaddrlen)) < 0)
            {
                fprintf(stderr,
                        "ERROR #5: error occured accepting connection.\n");
                exit(EXIT_FAILURE);
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
                    s_len =
                        recv(pfds[i].fd, usernames[i], sizeof(usernames[i]), 0);
                    if (s_len <= 0)
                    {
                        printf("Client disconnected.\n");
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
                        for (int j = 1; j <= MAX_CONNECTED_CLIENTS; j++)
                        {
                            if (pfds[j].fd != -1 && i != j)
                            {
                                r_len = send(pfds[j].fd, str, strlen(str), 0);
                            }
                        }
                    }
                }
                else
                {
                    printf("Trying to read from client %d (%d).\n", i,
                           pfds[i].fd);
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

    return 0;
}
