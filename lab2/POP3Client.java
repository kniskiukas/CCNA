package lab2;

import java.io.*;
import java.net.*;
import java.util.ArrayList;
import java.util.List;

/**
 * A simple POP3 client implementation in Java
 * POP3 (Post Office Protocol version 3) is used for retrieving emails from a mail server
 */
public class POP3Client {
    private Socket socket;
    private BufferedReader reader;
    private PrintWriter writer;
    private boolean connected = false;

    /**
     * Connect to the POP3 server
     * @param server POP3 server address
     * @param port POP3 server port (usually 110 for non-SSL, 995 for SSL)
     * @return Server's welcome message
     */
    public String connect(String server, int port) throws IOException {
        socket = new Socket(server, port);
        reader = new BufferedReader(new InputStreamReader(socket.getInputStream()));
        writer = new PrintWriter(socket.getOutputStream(), true);
        
        String response = reader.readLine();
        if (response.startsWith("+OK")) {
            connected = true;
            return response;
        } else {
            disconnect();
            throw new IOException("Connection failed: " + response);
        }
    }

    /**
     * Login to the POP3 server using username and password
     * @param username POP3 account username
     * @param password POP3 account password
     * @return true if login successful
     */
    public boolean login(String username, String password) throws IOException {
        if (!connected) {
            throw new IOException("Not connected to server");
        }
        
        // Send USER command
        writer.println("USER " + username);
        String response = reader.readLine();
        if (!response.startsWith("+OK")) {
            return false;
        }
        
        // Send PASS command
        writer.println("PASS " + password);
        response = reader.readLine();
        return response.startsWith("+OK");
    }

    /**
     * Get the number of messages and total size
     * @return int[] where [0] = message count, [1] = total size in bytes
     */
    public int[] getStatus() throws IOException {
        if (!connected) {
            throw new IOException("Not connected to server");
        }
        
        writer.println("STAT");
        String response = reader.readLine();
        
        if (response.startsWith("+OK")) {
            String[] parts = response.split(" ");
            if (parts.length >= 3) {
                int messageCount = Integer.parseInt(parts[1]);
                int totalSize = Integer.parseInt(parts[2]);
                return new int[]{messageCount, totalSize};
            }
        }
        
        throw new IOException("Failed to get status: " + response);
    }

    /**
     * List all messages with their sizes
     * @return List of message info (message number and size)
     */
    public List<String> listMessages() throws IOException {
        if (!connected) {
            throw new IOException("Not connected to server");
        }
        
        writer.println("LIST");
        String response = reader.readLine();
        
        if (!response.startsWith("+OK")) {
            throw new IOException("LIST command failed: " + response);
        }
        
        List<String> messages = new ArrayList<>();
        String line;
        while (!(line = reader.readLine()).equals(".")) {
            messages.add(line);
        }
        
        return messages;
    }

    /**
     * Retrieve a specific message
     * @param messageNumber The message number to retrieve
     * @return The message content
     */
    public String retrieveMessage(int messageNumber) throws IOException {
        if (!connected) {
            throw new IOException("Not connected to server");
        }
        
        writer.println("RETR " + messageNumber);
        String response = reader.readLine();
        
        if (!response.startsWith("+OK")) {
            throw new IOException("RETR command failed: " + response);
        }
        
        StringBuilder messageContent = new StringBuilder();
        String line;
        while (!(line = reader.readLine()).equals(".")) {
            // POP3 protocol: lines starting with "." have an extra "." that needs to be removed
            if (line.startsWith("..")) {
                line = line.substring(1);
            }
            messageContent.append(line).append("\n");
        }
        
        return messageContent.toString();
    }

    /**
     * Delete a message from the server
     * @param messageNumber The message number to delete
     * @return true if deletion was successful
     */
    public boolean deleteMessage(int messageNumber) throws IOException {
        if (!connected) {
            throw new IOException("Not connected to server");
        }
        
        writer.println("DELE " + messageNumber);
        String response = reader.readLine();
        
        return response.startsWith("+OK");
    }

    /**
     * Quit the POP3 session
     * @return Server's goodbye message
     */
    public String quit() throws IOException {
        if (!connected) {
            return "Not connected";
        }
        
        writer.println("QUIT");
        String response = reader.readLine();
        disconnect();
        
        return response;
    }

    /**
     * Disconnect from the server
     */
    public void disconnect() throws IOException {
        connected = false;
        if (reader != null) reader.close();
        if (writer != null) writer.close();
        if (socket != null && !socket.isClosed()) socket.close();
    }

    /**
     * Example usage of the POP3Client
     */
    public static void main(String[] args) {
        POP3Client client = new POP3Client();
        
        try {
            // Connect to server
            System.out.println("Connecting to server...");
            String welcome = client.connect("pop.example.com", 110);
            System.out.println("Server response: " + welcome);
            
            // Login
            System.out.println("Logging in...");
            boolean loginSuccess = client.login("username", "password");
            System.out.println("Login successful: " + loginSuccess);
            
            if (loginSuccess) {
                // Get mailbox status
                int[] status = client.getStatus();
                System.out.println("Messages: " + status[0] + ", Total size: " + status[1] + " bytes");
                
                // List all messages
                System.out.println("Listing messages:");
                List<String> messages = client.listMessages();
                for (String msg : messages) {
                    System.out.println(msg);
                }
                
                // Retrieve a specific message (e.g., the first one)
                if (status[0] > 0) {
                    System.out.println("Retrieving message 1:");
                    String message = client.retrieveMessage(1);
                    System.out.println(message);
                    
                    // Delete the message
                    System.out.println("Deleting message 1...");
                    boolean deleted = client.deleteMessage(1);
                    System.out.println("Message deleted: " + deleted);
                }
            }
            
            // Quit the session
            System.out.println("Quitting...");
            String goodbye = client.quit();
            System.out.println("Server response: " + goodbye);
            
        } catch (IOException e) {
            System.err.println("Error: " + e.getMessage());
            e.printStackTrace();
            
            try {
                client.disconnect();
            } catch (IOException ex) {
                ex.printStackTrace();
            }
        }
    }
}