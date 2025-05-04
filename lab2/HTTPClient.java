package lab2;

import java.util.Map;
import java.util.Scanner;
import java.util.List;
import java.net.URL;
import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class HTTPClient
{
    private int statusCode;
    private String statusMessage;
    private String body;
    private Map<String, List<String>> headers;

    public HTTPClient(int statusCode, String statusMessage, String body, Map<String, List<String>> headers)
    {
        this.statusCode = statusCode;
        this.statusMessage = statusMessage;
        this.body = body;
        this.headers = headers;
    }

    public int getStatusCode()
    {
        return statusCode;
    }

    public String getStatusMessage()
    {
        return statusMessage;
    }

    public String getBody()
    {
        return body;
    }

    public Map<String, List<String>> getHeaders()
    {
        return headers;
    }

    public List<String> getHeader(String name)
    {
        return headers.get(name.toLowerCase());
    }

    public String getFirstHeader(String name)
    {
        List<String> values = headers.get(name.toLowerCase());
        return values != null && !values.isEmpty() ? values.get(0) : null;
    }

    public HTTPClient sendGetRequest(String urlString) throws IOException
    {
        URL url = new URL(urlString);
        String host = url.getHost();
        int port = url.getPort() == -1 ? 80 : url.getPort();
        String path = url.getPath().isEmpty() ? "/" : url.getPath();

        try (Socket socket = new Socket(host, port);
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
             BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream())))
        {
            // Send the GET request
            out.println("GET " + path + " HTTP/1.1");
            out.println("Host: " + host);
            out.println("Connection: close");
            out.println(); // End of headers

            // Read the response
            String statusLine = in.readLine();
            if (statusLine == null)
            {
                throw new IOException("Received an empty response from the server.");
            }

            String[] statusParts = statusLine.split(" ", 3);
            int statusCode = Integer.parseInt(statusParts[1]);
            String statusMessage = statusParts.length > 2 ? statusParts[2] : "";

            java.util.Map<String, java.util.List<String>> responseHeaders = new java.util.HashMap<>();
            String headerLine;
            while ((headerLine = in.readLine()) != null && !headerLine.isEmpty())
            {
                String[] headerParts = headerLine.split(":", 2);
                if (headerParts.length == 2)
                {
                    String headerName = headerParts[0].trim().toLowerCase();
                    String headerValue = headerParts[1].trim();
                    responseHeaders.computeIfAbsent(headerName, k -> new java.util.ArrayList<>()).add(headerValue);
                }
            }

            StringBuilder responseBody = new StringBuilder();
            String bodyLine;
            while ((bodyLine = in.readLine()) != null)
            {
                responseBody.append(bodyLine).append("\n");
            }
            // Remove the last newline if the body is not empty
            String body = responseBody.length() > 0 ? responseBody.toString().trim() : "";

            return new HTTPClient(statusCode, statusMessage, body, responseHeaders);
        }
    }

    public HTTPClient sendPostRequest(String urlString, String postData) throws IOException
    {
        URL url = new URL(urlString);
        String host = url.getHost();
        int port = url.getPort() == -1 ? 80 : url.getPort();
        String path = url.getPath().isEmpty() ? "/" : url.getPath();

        try (Socket socket = new Socket(host, port);
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
             BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream())))
        {
            // Send the POST request
            out.println("POST " + path + " HTTP/1.1");
            out.println("Host: " + host);
            out.println("Content-Type: application/x-www-form-urlencoded");
            out.println("Content-Length: " + postData.length());
            out.println("Connection: close");
            out.println(); // End of headers
            out.println(postData); // Send the body

            // Read the response
            String statusLine = in.readLine();
            if (statusLine == null)
            {
                throw new IOException("Received an empty response from the server.");
            }

            String[] statusParts = statusLine.split(" ", 3);
            int statusCode = Integer.parseInt(statusParts[1]);
            String statusMessage = statusParts.length > 2 ? statusParts[2] : "";

            java.util.Map<String, java.util.List<String>> responseHeaders = new java.util.HashMap<>();
            String headerLine;
            while ((headerLine = in.readLine()) != null && !headerLine.isEmpty())
            {
                String[] headerParts = headerLine.split(":", 2);
                if (headerParts.length == 2)
                {
                    String headerName = headerParts[0].trim().toLowerCase();
                    String headerValue = headerParts[1].trim();
                    responseHeaders.computeIfAbsent(headerName, k -> new java.util.ArrayList<>()).add(headerValue);
                }
            }

            StringBuilder responseBody = new StringBuilder();
            String bodyLine;
            while ((bodyLine = in.readLine()) != null)
            {
                responseBody.append(bodyLine).append("\n");
            }
            // Remove the last newline if the body is not empty
            String body = responseBody.length() > 0 ? responseBody.toString().trim() : "";

            return new HTTPClient(statusCode, statusMessage, body, responseHeaders);
        }
    }

    public HTTPClient sendPutRequest(String urlString, String putData) throws IOException
    {
        URL url = new URL(urlString);
        String host = url.getHost();
        int port = url.getPort() == -1 ? 80 : url.getPort();
        String path = url.getPath().isEmpty() ? "/" : url.getPath();

        try (Socket socket = new Socket(host, port);
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
             BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream())))
        {
            // Send the PUT request
            out.println("PUT " + path + " HTTP/1.1");
            out.println("Host: " + host);
            out.println("Content-Type: application/x-www-form-urlencoded");
            out.println("Content-Length: " + putData.length());
            out.println("Connection: close");
            out.println(); // End of headers
            out.println(putData); // Send the body

            // Read the response
            String statusLine = in.readLine();
            if (statusLine == null)
            {
                throw new IOException("Received an empty response from the server.");
            }

            String[] statusParts = statusLine.split(" ", 3);
            int statusCode = Integer.parseInt(statusParts[1]);
            String statusMessage = statusParts.length > 2 ? statusParts[2] : "";

            java.util.Map<String, java.util.List<String>> responseHeaders = new java.util.HashMap<>();
            String headerLine;
            while ((headerLine = in.readLine()) != null && !headerLine.isEmpty())
            {
                String[] headerParts = headerLine.split(":", 2);
                if (headerParts.length == 2)
                {
                    String headerName = headerParts[0].trim().toLowerCase();
                    String headerValue = headerParts[1].trim();
                    responseHeaders.computeIfAbsent(headerName, k -> new java.util.ArrayList<>()).add(headerValue);
                }
            }

            StringBuilder responseBody = new StringBuilder();
            String bodyLine;
            while ((bodyLine = in.readLine()) != null)
            {
                responseBody.append(bodyLine).append("\n");
            }
            // Remove the last newline if the body is not empty
            String body = responseBody.length() > 0 ? responseBody.toString().trim() : "";

            return new HTTPClient(statusCode, statusMessage, body, responseHeaders);
        }
    }

    public HTTPClient sendDeleteRequest(String urlString) throws IOException
    {
        URL url = new URL(urlString);
        String host = url.getHost();
        int port = url.getPort() == -1 ? 80 : url.getPort();
        String path = url.getPath().isEmpty() ? "/" : url.getPath();

        try (Socket socket = new Socket(host, port);
             PrintWriter out = new PrintWriter(socket.getOutputStream(), true);
             BufferedReader in = new BufferedReader(new InputStreamReader(socket.getInputStream())))
        {
            // Send the DELETE request
            out.println("DELETE " + path + " HTTP/1.1");
            out.println("Host: " + host);
            out.println("Connection: close");
            out.println(); // End of headers

            // Read the response
            String statusLine = in.readLine();
            if (statusLine == null)
            {
                throw new IOException("Received an empty response from the server.");
            }

            String[] statusParts = statusLine.split(" ", 3);
            int statusCode = Integer.parseInt(statusParts[1]);
            String statusMessage = statusParts.length > 2 ? statusParts[2] : "";

            java.util.Map<String, java.util.List<String>> responseHeaders = new java.util.HashMap<>();
            String headerLine;
            while ((headerLine = in.readLine()) != null && !headerLine.isEmpty())
            {
                String[] headerParts = headerLine.split(":", 2);
                if (headerParts.length == 2)
                {
                    String headerName = headerParts[0].trim().toLowerCase();
                    String headerValue = headerParts[1].trim();
                    responseHeaders.computeIfAbsent(headerName, k -> new java.util.ArrayList<>()).add(headerValue);
                }
            }

            StringBuilder responseBody = new StringBuilder();
            String bodyLine;
            while ((bodyLine = in.readLine()) != null)
            {
                responseBody.append(bodyLine).append("\n");
            }
            // Remove the last newline if the body is not empty
            String body = responseBody.length() > 0 ? responseBody.toString().trim() : "";

            return new HTTPClient(statusCode, statusMessage, body, responseHeaders);
        }
    }
    
    @Override
    public String toString()
    {
        StringBuilder sb = new StringBuilder();
        sb.append("Status: ").append(statusCode).append(" ").append(statusMessage).append("\r\n");
        sb.append("Headers:\r\n");
        for (Map.Entry<String, List<String>> entry : headers.entrySet())
        {
            for (String value : entry.getValue())
            {
                sb.append(entry.getKey()).append(": ").append(value).append("\r\n");
            }
        }
        sb.append("Body Length: ").append(body.length()).append("\r\n");
        sb.append("Body:\r\n");
        sb.append("\r\n").append(body);
        return sb.toString();
    }
    public static void main(String[] args)
    {
        Scanner scanner = new Scanner(System.in);
        HTTPClient client = new HTTPClient(0, "", "", null);
        HTTPClient response = null;

        System.out.println("Enter an option:");
        System.out.println("1: Enter URL for GET request");
        System.out.println("2: Run GET example.com test");
        System.out.println("3: Run GET 404 status test");
        System.out.println("4: Run POST request test");
        System.out.println("5: Run PUT request test");
        System.out.println("6: Run DELETE request test");
        System.out.println("7: Exit");

        while (true)
        {
            System.out.print("> ");
            String choice = scanner.nextLine();

            try
            {
                switch (choice)
                {
                    case "1":
                        System.out.println("Enter URL for GET request:");
                        System.out.print("URL> ");
                        String urlInput = scanner.nextLine();
                        response = client.sendGetRequest(urlInput);
                        System.out.println("\nResponse for: " + urlInput);
                        System.out.println(response.toString());
                        break;
                    case "2":
                        System.out.println("\nRunning GET example.com test:");
                        response = client.sendGetRequest("http://example.com");
                        System.out.println(response.toString());
                        if (response.getStatusCode() == 200) System.out.println("Test passed!");
                        else System.out.println("Test failed. Status code: " + response.getStatusCode());
                        break;
                    case "3":
                        System.out.println("\nRunning GET 404 status test:");
                        response = client.sendGetRequest("https://httpbin.org/status/404");
                        System.out.println(response.toString());
                        if (response.getStatusCode() == 404) System.out.println("Test passed!");
                        else System.out.println("Test failed. Status code: " + response.getStatusCode());
                        break;
                    case "4":
                        System.out.println("\nRunning POST request test:");
                        String postData = "name=Augustas&age=21";
                        response = client.sendPostRequest("https://httpbin.org/post", postData);
                        System.out.println(response.toString());
                        if (response.getStatusCode() == 200 && response.getBody().contains(postData)) System.out.println("Test passed!");
                        else System.out.println("Test failed. Status code: " + response.getStatusCode() + ", Body: " + response.getBody());
                        break;
                    case "5":
                        System.out.println("\nRunning PUT request test:");
                        String putData = "name=Augustas&age=22";
                        response = client.sendPutRequest("https://httpbin.org/put", putData);
                        System.out.println(response.toString());
                        if (response.getStatusCode() == 200 && response.getBody().contains(putData)) System.out.println("Test passed!");
                        else System.out.println("Test failed. Status code: " + response.getStatusCode() + ", Body: " + response.getBody());
                        break;
                    case "6":
                        System.out.println("\nRunning DELETE request test:");
                        response = client.sendDeleteRequest("https://httpbin.org/delete");
                        System.out.println(response.toString());
                        if (response.getStatusCode() == 200) System.out.println("Test passed!");
                        else System.out.println("Test failed. Status code: " + response.getStatusCode());
                        break;
                    case "7":
                        System.out.println("Exiting HTTP Client.");
                        scanner.close();
                        return;
                    default:
                        System.out.println("Invalid option. Please enter a number between 1 and 7.");
                }
                System.out.println("--------------------");
            } catch (IOException e)
            {
                System.err.println("Error: " + e.getMessage());
                e.printStackTrace();
            }
            System.out.println("\nEnter an option (1-7):");
        }
    }
}