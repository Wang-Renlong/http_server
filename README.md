# Simple Http Server

**A simple HTTP server  implemented by C language.** 
- Using single thread.
- Only support GET method.
- build on Visual Studio 2013 without secure check, so I using `fopen` instead of `fopen_s`.

**For Linux and other platforms:**
Just rewrite the functions in sock.c and needn't change any function call in main.c or sock.h.

**Usage:**
Execute `http_server_c -h` in console and you can see such message:
```
Simple HTTP Server v1.0
Usage: httpserver ip:port [-t timeout] [-p respath]
        ip       server ip address
        port     server port
        -t       waiting time when recving data
        timeout  number bigger than 0
        -p       resource file(s) path
        respath  a valid local path no more than 512 bytes
help:
    -h       show help message

```
To start the server, execute `http_server_c ip:port -t timeout -p resourcepath`
- ip is your ip address.
- port is the port you want to bind. 
- timeout is the waiting time when the server can't get data from a client socket.
- resourcepath is your static webset path.
you should add '"' at the start and end of your resource path if your path contains space character.
