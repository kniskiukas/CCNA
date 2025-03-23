/*
 * Drawing Board Server (No Threading)
 * 
 * Author: Original by Kęstutis Mizara, Modified for Drawing Board
 * Description: Manages a shared drawing board for multiple clients using select()
 */

 #ifdef _WIN32
 #include <winsock2.h>
 #define socklen_t int
 #else
 #include <sys/socket.h>
 #include <sys/types.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>
 #include <sys/select.h>
 #endif
 
 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <time.h>
 
 #define MAX_CLIENTS 10
 #define BUFFER_SIZE 1024
 #define MAX_DRAW_POINTS 1000
 
 // Structure to represent a drawing point
 typedef struct {
     int x;
     int y;
     int color;
     int client_id;
 } DrawPoint;
 
 // Global variables for the drawing state
 DrawPoint drawing_state[MAX_DRAW_POINTS];
 int point_count = 0;
 
 // Array of client sockets
 int client_sockets[MAX_CLIENTS];
 int client_count = 0;
 
 // Parse a drawing command from the client
 // Format: "DRAW x y color"
 int parse_draw_command(char *buffer, DrawPoint *point) {
     char command[10];
     int result = sscanf(buffer, "%s %d %d %d", command, &point->x, &point->y, &point->color);
     
     if (result != 4 || strcmp(command, "DRAW") != 0) {
         return 0;
     }
     
     return 1;
 }
 
 // Broadcast a drawing update to all clients
 void broadcast_draw_update(DrawPoint *point, int exclude_socket) {
     char buffer[BUFFER_SIZE];
     snprintf(buffer, BUFFER_SIZE, "DRAW %d %d %d %d", point->x, point->y, point->color, point->client_id);
     
     for (int i = 0; i < client_count; i++) {
         if (client_sockets[i] != exclude_socket && client_sockets[i] > 0) {
             send(client_sockets[i], buffer, strlen(buffer), 0);
         }
     }
 }
 
 // Send the entire drawing state to a new client
 void send_full_state(int socket, int client_id) {
     char buffer[BUFFER_SIZE];
     snprintf(buffer, BUFFER_SIZE, "WELCOME %d", client_id);
     send(socket, buffer, strlen(buffer), 0);
     
     for (int i = 0; i < point_count; i++) {
         snprintf(buffer, BUFFER_SIZE, "DRAW %d %d %d %d", 
                 drawing_state[i].x, drawing_state[i].y, 
                 drawing_state[i].color, drawing_state[i].client_id);
         send(socket, buffer, strlen(buffer), 0);
         
         // Small delay to prevent overwhelming client
         #ifdef _WIN32
         Sleep(10);
         #else
         struct timespec ts;
         ts.tv_sec = 0;
         ts.tv_nsec = 10000000; // 10ms
         nanosleep(&ts, NULL);
         #endif
     }
 }
 
 // Add a client socket to our array
 int add_client(int socket) {
     int client_id = -1;
     
     if (client_count < MAX_CLIENTS) {
         client_sockets[client_count] = socket;
         client_id = client_count;
         client_count++;
     }
     
     return client_id;
 }
 
 // Remove a client socket from our array
 void remove_client(int socket) {
     for (int i = 0; i < client_count; i++) {
         if (client_sockets[i] == socket) {
             // Shift remaining clients
             for (int j = i; j < client_count - 1; j++) {
                 client_sockets[j] = client_sockets[j + 1];
             }
             client_count--;
             break;
         }
     }
 }
 
 int main(int argc, char *argv[]) {
 #ifdef _WIN32
     WSADATA data;
 #endif
     unsigned int port;
     int l_socket; // socket for accepting connections
     
     struct sockaddr_in servaddr; // Server address structure
     
     if (argc != 2) {
         printf("USAGE: %s <port>\n", argv[0]);
         exit(1);
     }
     
     port = atoi(argv[1]);
     
     if ((port < 1) || (port > 65535)) {
         printf("ERROR #1: invalid port specified.\n");
         exit(1);
     }
     
 #ifdef _WIN32
     WSAStartup(MAKEWORD(2,2),&data);    
 #endif
     
     // Create server socket
     if ((l_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
         fprintf(stderr, "ERROR #2: cannot create listening socket.\n");
         exit(1);
     }
     
     // Initialize server address structure
     memset(&servaddr, 0, sizeof(servaddr));
     servaddr.sin_family = AF_INET;
     servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
     servaddr.sin_port = htons(port);
     
     // Bind socket to address
     if (bind(l_socket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
         fprintf(stderr, "ERROR #3: bind listening socket.\n");
         exit(1);
     }
     
     // Listen for connections
     if (listen(l_socket, 10) < 0) {
         fprintf(stderr, "ERROR #4: error in listen().\n");
         exit(1);
     }
     
     printf("Drawing board server started on port %d\n", port);
     
     // Initialize socket sets for select()
     fd_set master_set, read_set;
     FD_ZERO(&master_set);
     FD_SET(l_socket, &master_set);
     int max_socket = l_socket;
     
     // Main event loop
     while (1) {
         read_set = master_set;
         
         if (select(max_socket + 1, &read_set, NULL, NULL, NULL) < 0) {
             fprintf(stderr, "ERROR: select() failed\n");
             break;
         }
         
         // Check for new connections
         if (FD_ISSET(l_socket, &read_set)) {
             struct sockaddr_in clientaddr;
             socklen_t clientaddrlen = sizeof(struct sockaddr);
             
             int c_socket;
             if ((c_socket = accept(l_socket, (struct sockaddr*)&clientaddr, &clientaddrlen)) < 0) {
                 fprintf(stderr, "ERROR #5: error accepting connection.\n");
                 continue;
             }
             
             printf("New client connected from %s\n", inet_ntoa(clientaddr.sin_addr));
             
             // Add to client list
             int client_id = add_client(c_socket);
             if (client_id < 0) {
                 printf("Too many clients, connection refused.\n");
                 #ifdef _WIN32
                 closesocket(c_socket);
                 #else
                 close(c_socket);
                 #endif
                 continue;
             }
             
             // Add to master set
             FD_SET(c_socket, &master_set);
             if (c_socket > max_socket) {
                 max_socket = c_socket;
             }
             
             // Send current drawing state
             send_full_state(c_socket, client_id);
         }
         
         // Check for data from clients
         for (int i = 0; i < client_count; i++) {
             int socket = client_sockets[i];
             
             if (FD_ISSET(socket, &read_set)) {
                 char buffer[BUFFER_SIZE];
                 memset(buffer, 0, BUFFER_SIZE);
                 
                 int bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
                 
                 if (bytes_received <= 0) {
                     // Client disconnected
                     printf("Client %d disconnected\n", i);
                     FD_CLR(socket, &master_set);
                     #ifdef _WIN32
                     closesocket(socket);
                     #else
                     close(socket);
                     #endif
                     remove_client(socket);
                 } else {
                     // Process drawing command
                     DrawPoint new_point;
                     new_point.client_id = i;
                     
                     if (parse_draw_command(buffer, &new_point)) {
                         // Add to drawing state
                         if (point_count < MAX_DRAW_POINTS) {
                             drawing_state[point_count] = new_point;
                             point_count++;
                         }
                         
                         // Broadcast to other clients
                         broadcast_draw_update(&new_point, socket);
                     }
                 }
             }
         }
     }
     
     // Clean up
     #ifdef _WIN32
     closesocket(l_socket);
     WSACleanup();
     #else
     close(l_socket);
     #endif
     
     return 0;
 }