// fix the client.c file so that the board is 80 in x and 20 in y and the commands first take x, not y

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define CANVAS_WIDTH 21
#define CANVAS_HEIGHT 21
#define MAX_USERNAME_LENGTH 15

// Structure to represent the local drawing canvas
typedef struct
{
    char grid[CANVAS_HEIGHT][CANVAS_WIDTH];
} Canvas;

Canvas canvas;
int s_socket;           // Server socket
int should_display = 0; // Flag to control board display

// Initialize the canvas with spaces
void init_canvas()
{
    for (int y = 0; y < CANVAS_HEIGHT; y++)
    {
        for (int x = 0; x < CANVAS_WIDTH; x++)
        {
            canvas.grid[y][x] = ' ';
        }
    }
}

void client_info_display()
{
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif

    printf("\nCommands: /draw x y symbol (e.g., '/draw 5 10 #' to draw)\n");
    printf("         /show (show board)\n");
    printf("         /reset (reset board)\n");
    printf("         /help (show commands)\n");
    printf("         /exit (to exit)\n");
    printf("Enter command: ");
    fflush(stdout);
}



// Update the local canvas based on server update
void update_local_canvas(char *board_string)
{
    int index = 0;
    for (int y = 0; y < CANVAS_HEIGHT; y++)
    {
        for (int x = 0; x < CANVAS_WIDTH; x++)
        {
            if (index < strlen(board_string))
            {
                canvas.grid[y][x] = board_string[index++];
            }
            else
            {
                return; // Prevent out-of-bounds access
            }
        }
        if (index < strlen(board_string) && board_string[index] == '\n')
        {
            index++;
        }
    }
}

// Send a command to the server
void send_command_to_server(char *command)
{
    send(s_socket, command, strlen(command), 0);
    printf("\nClient sent: %s\n", command);
}

// Set up non-blocking input
void setup_nonblocking_input()
{
#ifdef _WIN32
#else
    int flags = fcntl(0, F_GETFL, 0);
    fcntl(0, F_SETFL, flags | O_NONBLOCK);
#endif
}

int main(int argc, char *argv[])
{
#ifdef _WIN32
    WSADATA data;
#endif
    unsigned int port;
    struct sockaddr_in servaddr; // Server address structure
    printf("Connecting...\n");
    fflush(stdout);

    if (argc != 3)
    {
        fprintf(stderr, "USAGE: %s <ip> <port>\n", argv[0]);
        exit(1);
    }
    printf("Connecting to server at %s:%s\n", argv[1], argv[2]);
    fflush(stdout);

    port = atoi(argv[2]);

    if ((port < 1) || (port > 65535))
    {
        fprintf(stderr, "Invalid port number: %s\n", argv[2]);
        exit(1);
    }

    // Initialize Winsock for Windows
#ifdef _WIN32
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
    {
        fprintf(stderr, "Failed to initialize Winsock\n");
        exit(1);
    }
#endif

    // Create socket
    if ((s_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Could not create socket\n");
#ifdef _WIN32
        WSACleanup();
#endif
        exit(1);
    }

    // Prepare the server address structure
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);

    // Convert IP address from text to binary form
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
    {
        fprintf(stderr, "Invalid address: %s\n", argv[1]);
#ifdef _WIN32
        closesocket(s_socket);
        WSACleanup();
#else
        close(s_socket);
#endif
        exit(1);
    }

    // Connect to server
    if (connect(s_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0)
    {
        fprintf(stderr, "Connection failed\n");
#ifdef _WIN32
        closesocket(s_socket);
        WSACleanup();
#else
        close(s_socket);
#endif
        exit(1);
    }

    printf("Connected to server at %s:%d\n", argv[1], port);

    // Initialize canvas
    init_canvas();

    // Get username from the user
    char username[MAX_USERNAME_LENGTH];
    printf("Enter your username: ");
    fgets(username, MAX_USERNAME_LENGTH, stdin);
    username[strcspn(username, "\n")] = 0; // Remove trailing newline

    // Send the username to the server
    send(s_socket, username, strlen(username), 0);

    // Set up non block
    setup_nonblocking_input();

    char buffer[BUFFER_SIZE];
    char input_buffer[BUFFER_SIZE];
    char *input_ptr = input_buffer;
    input_buffer[0] = '\0';
    int bytes_received;

    bytes_received = recv(s_socket, buffer, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0)
    {
        buffer[bytes_received] = '\0';
        printf("%s\n", buffer);
    }

    client_info_display();

    fd_set readfds;
    struct timeval tv;

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(s_socket, &readfds);
        FD_SET(0, &readfds); // stdin

        // Set timeout for select
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int activity = select(s_socket + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0)
        {
            fprintf(stderr, "Select error\n");
            break;
        }

        // Check for data from server
        if (FD_ISSET(s_socket, &readfds))
        {
            bytes_received = recv(s_socket, buffer, BUFFER_SIZE - 1, 0);
            if (bytes_received > 0)
            {
                buffer[bytes_received] = '\0';
                // Check if the received data is a board update
                if (strncmp(buffer, "BOARD:", 6) == 0)
                {
                    update_local_canvas(buffer + 6); // Update canvas without "BOARD:" prefix
                    if (should_display)
                    {
                        client_info_display();
                        should_display = 0; // Reset the flag
                    }
                }
                else
                {
                    printf("\nServer says:\n %s\n", buffer); // Print other messages from the server
                    printf("Enter command: ");
                    fflush(stdout);
                }
            }
            else if (bytes_received == 0)
            {
                printf("Server disconnected.\n");
                break;
            }
            else
            {
                perror("recv");
                break;
            }
        }

        // Check for user input
        if (FD_ISSET(0, &readfds))
        {
            char ch;
            int read_size = read(0, &ch, 1);

            if (read_size > 0)
            {
                if (ch == '\n')
                {
                    // Process command
                    input_buffer[strlen(input_buffer)] = '\0'; // Ensure null termination
                    if (strcmp(input_buffer, "/exit") == 0)
                    {
                        printf("\nExiting...\n");
                        break;
                    }
                    else if (strcmp(input_buffer, "/show") == 0)
                    {
                        should_display = 1;                   // Set the flag to display the board
                        send_command_to_server(input_buffer); // Still send /show to the server
                        client_info_display();                // Display immediately on local command
                    }
                    else
                    {
                        // Send the entire command to the server
                        send_command_to_server(input_buffer);
                        // If the command was a draw command, we might want to display immediately
                        if (strncmp(input_buffer, "/draw", 5) == 0)
                        {
                            should_display = 1; // Set flag to display after server confirms
                        }
                        printf("Enter command: ");
                        fflush(stdout);
                    }

                    // Reset input buffer
                    input_ptr = input_buffer;
                    *input_ptr = '\0';
                }
                else if (ch == 127 || ch == 8)
                { // Backspace
                    if (input_ptr > input_buffer)
                    {
                        input_ptr--;
                        *input_ptr = '\0';
                        // Redraw input line with updated buffer
                        printf("\rEnter command: %s \b", input_buffer);
                        fflush(stdout);
                    }
                }
                else
                {
                    // Add character to buffer if there's room
                    if (input_ptr - input_buffer < BUFFER_SIZE - 1)
                    {
                        *input_ptr++ = ch;
                        *input_ptr = '\0';
                        // Echo character
                        putchar(ch);
                        fflush(stdout);
                    }
                }
            }
        }
    }

    // Clean up
#ifdef _WIN32
    closesocket(s_socket);
    WSACleanup();
#else
    close(s_socket);
#endif

    return 0;
}