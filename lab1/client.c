/*
 * Drawing Board Client (No Threading)
 * 
 * Author: Original by Kęstutis Mizara, Modified for Drawing Board
 * Description: Connects to drawing board server and exchanges drawing commands
 */

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
 #define CANVAS_WIDTH 51
 #define CANVAS_HEIGHT 21
 
 // Structure to represent the local drawing canvas
 typedef struct {
     char grid[CANVAS_HEIGHT][CANVAS_WIDTH];
     int client_id;
     int curr_color;
 } Canvas;
 
 Canvas canvas;
 int s_socket; // Server socket
 
 // Initialize the canvas with spaces
 void init_canvas() {
     for (int y = 0; y < CANVAS_HEIGHT; y++) {
         for (int x = 0; x < CANVAS_WIDTH; x++) {
             canvas.grid[y][x] = ' ';
         }
     }
     canvas.client_id = -1;
     canvas.curr_color = 1; // Default color
 }
 
 // Display the canvas
 void display_canvas() {
     #ifdef _WIN32
     system("cls");
     #else
     system("clear");
     #endif
     
     // Print top border
     printf("+");
     for (int x = 0; x < CANVAS_WIDTH; x++) {
         printf("-");
     }
     printf("+\n");
     
     // Print grid with side borders
     for (int y = 0; y < CANVAS_HEIGHT; y++) {
         printf("|");
         for (int x = 0; x < CANVAS_WIDTH; x++) {
             printf("%c", canvas.grid[y][x]);
         }
         printf("|\n");
     }
     
     // Print bottom border
     printf("+");
     for (int x = 0; x < CANVAS_WIDTH; x++) {
         printf("-");
     }
     printf("+\n");
     
     // Print client info and help
     printf("You are client #%d. Current color: %d\n", canvas.client_id, canvas.curr_color);
     printf("Commands: x y c (e.g., '5 10 3' to draw at x=5, y=10 with color 3)\n");
     printf("         'quit' to exit\n");
     printf("Enter command: ");
     fflush(stdout);
 }
 
 // Parse a DRAW command from the server
 // Format: "DRAW x y color client_id"
 
 // Parse a WELCOME command from the server
 // Format: "WELCOME client_id"
 int parse_welcome_command(char *buffer, int *id) {
     char command[10];
     int result = sscanf(buffer, "%s %d", command, id);
     
     if (result != 2 || strcmp(command, "WELCOME") != 0) {
         return 0;
     }
     
     return 1;
 }
 
 // Update the canvas with a new drawing point
 void update_canvas(int x, int y, int color, int client_id) {
     if (x < 0 || x >= CANVAS_WIDTH || y < 0 || y >= CANVAS_HEIGHT) {
         return; // Out of bounds
     }
     
     // Different clients use different characters based on color
     char draw_char;
     switch (color % 10) {
         case 0: draw_char = ' '; break; // Eraser
         case 1: draw_char = '*'; break;
         case 2: draw_char = '+'; break;
         case 3: draw_char = 'o'; break;
         case 4: draw_char = '#'; break;
         case 5: draw_char = '@'; break;
         case 6: draw_char = '='; break;
         case 7: draw_char = '.'; break;
         case 8: draw_char = ':'; break;
         case 9: draw_char = '$'; break;
         default: draw_char = '*';
     }
     
     canvas.grid[y][x] = draw_char;
 }
 
 // Send a drawing command to the server
 // Send a drawing command to the server
void send_draw_command(int x, int y, int color) {
    char buffer[BUFFER_SIZE];
    char symbol;

    // Determine symbol based on color (same logic as update_canvas)
    switch (color % 10) {
        case 0: symbol = ' '; break; // Eraser
        case 1: symbol = '*'; break;
        case 2: symbol = '+'; break;
        case 3: symbol = 'o'; break;
        case 4: symbol = '#'; break;
        case 5: symbol = '@'; break;
        case 6: symbol = '='; break;
        case 7: symbol = '.'; break;
        case 8: symbol = ':'; break;
        case 9: symbol = '$'; break;
        default: symbol = '*';
    }

    snprintf(buffer, BUFFER_SIZE, "/draw %d %d %c", x, y, symbol);
    send(s_socket, buffer, strlen(buffer), 0);
}
 
 // Set up non-blocking input
 void setup_nonblocking_input() {
     #ifdef _WIN32
     // Windows doesn't support setting stdin to non-blocking in a standard way
     // We'll use select() with a timeout instead
     #else
     int flags = fcntl(0, F_GETFL, 0);
     fcntl(0, F_SETFL, flags | O_NONBLOCK);
     #endif
 }
 
 int main(int argc, char *argv[]) {
 #ifdef _WIN32
     WSADATA data;
 #endif    
     unsigned int port;
     struct sockaddr_in servaddr; // Server address structure
     
     if (argc != 3) {
         fprintf(stderr,"USAGE: %s <ip> <port>\n", argv[0]);
         exit(1);
     }
     
     port = atoi(argv[2]);
     
     if ((port < 1) || (port > 65535)) {
    fprintf(stderr,"Invalid port number: %s\n", argv[2]);
    exit(1);
}

// Initialize Winsock for Windows
#ifdef _WIN32
if (WSAStartup(MAKEWORD(2,2), &data) != 0) {
    fprintf(stderr, "Failed to initialize Winsock\n");
    exit(1);
}
#endif

// Create socket
if ((s_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0) {
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
if (connect(s_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
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

// Set up non-blocking input
setup_nonblocking_input();

char buffer[BUFFER_SIZE];
char input_buffer[BUFFER_SIZE];
char *input_ptr = input_buffer;
input_buffer[0] = '\0';

// Wait for welcome message to get client ID
int bytes_received = recv(s_socket, buffer, BUFFER_SIZE - 1, 0);
if (bytes_received > 0) {
    buffer[bytes_received] = '\0';
    int client_id;
    if (parse_welcome_command(buffer, &client_id)) {
        canvas.client_id = client_id;
        printf("Assigned client ID: %d\n", client_id);
    }
}

display_canvas();

fd_set readfds;
struct timeval tv;

while (1) {
    FD_ZERO(&readfds);
    FD_SET(s_socket, &readfds);
    FD_SET(0, &readfds); // stdin
    
    // Set timeout for select
    tv.tv_sec = 0;
    tv.tv_usec = 100000; // 100ms
    
    int activity = select(s_socket + 1, &readfds, NULL, NULL, &tv);
    
    if (activity < 0) {
        fprintf(stderr, "Select error\n");
        break;
    }
    
    // Check for data from server
    if (FD_ISSET(s_socket, &readfds)) {
        bytes_received = recv(s_socket, buffer, BUFFER_SIZE - 1, 0);
        
        // ... inside the while(1) loop, in the FD_ISSET(s_socket, &readfds) block ...

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("Received from server: %s", buffer); // For debugging

            // Check if the message is "Board reset."
            if (strcmp(buffer, "Board reset.\n") == 0) {
                init_canvas();
                display_canvas();
            }
            // Otherwise, assume it's the board string
            else {
                int index = 0;
                for (int y = 0; y < CANVAS_HEIGHT; y++) { // Make sure CANVAS_HEIGHT is 20
                    for (int x = 0; x < CANVAS_WIDTH; x++) { // Make sure CANVAS_WIDTH is 20
                        if (index < bytes_received) {
                            if (buffer[index] != '\n') {
                                canvas.grid[y][x] = buffer[index];
                            } else {
                                x--; // Don't increment x for newline
                            }
                            index++;
                        } else {
                            break; // Handle potential truncation
                        }
                    }
                    if (index < bytes_received && buffer[index] == '\n') {
                        index++; // Skip newline
                    }
                    if (index >= bytes_received) break;
                }
                display_canvas();
            }
        }
    }
    
    // Check for user input
    if (FD_ISSET(0, &readfds)) {
        char ch;
        int read_size = read(0, &ch, 1);
        
        if (read_size > 0) {
            if (ch == '\n') {
                // Process command
                if (strcmp(input_buffer, "quit") == 0) {
                    printf("Exiting...\n");
                    break;
                } else {
                    int x, y, color;
                    if (sscanf(input_buffer, "%d %d %d", &x, &y, &color) == 3) {
                        canvas.curr_color = color;
                        send_draw_command(x, y, color);
                    } else {
                        printf("Invalid command format. Use: x y color\n");
                    }
                }
                
                // Reset input buffer
                input_ptr = input_buffer;
                *input_ptr = '\0';
                display_canvas();
            } else if (ch == 127 || ch == 8) { // Backspace
                if (input_ptr > input_buffer) {
                    input_ptr--;
                    *input_ptr = '\0';
                    // Redraw input line with updated buffer
                    printf("\rEnter command: %s \b", input_buffer);
                    fflush(stdout);
                }
            } else {
                // Add character to buffer if there's room
                if (input_ptr - input_buffer < BUFFER_SIZE - 1) {
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