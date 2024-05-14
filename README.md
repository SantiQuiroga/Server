# HTTP Server

This is a simple HTTP server written in C. It supports GET and POST requests and can serve static files as well as execute Python scripts.

## How to Use

1. Run the compiled program:

```bash
./server.sh -p <port>
```

The server will start and listen for connections on port the port specified.

## Using the Server with Chrome

Once the server is running, you can interact with it using any web browser, including Chrome. Simply open Chrome and navigate to `http://localhost:<port>`.

If you want to request a specific file, append the filename to the URL. For example, to request a file named `index.html`, navigate to `http://localhost:<port>/index.html`.

However, the server will be exposed to the internet using Ngrok and the link will be provided by request.

## User Guide

The server can serve static files and execute Python scripts. The files and scripts should be located in the server directory.

- To request a static file, use a GET request. The server will return the contents of the file. If the file does not exist, the server will return a 404 Not Found error.

- To execute a Python script, use a POST request. The server will execute the script and return the output. If the script does not exist or fails to execute, the server will return an appropriate error.
