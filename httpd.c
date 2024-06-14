#include "httpd.h"

int main(int argc, char const *argv[])
{
    int port = 0;
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <TCP port number>\n", argv[0]);
        return EXIT_FAILURE;
    }
    else
    {
        port = atoi(argv[1]);
        if (port < 1024 || port > 65535)
        {
            fprintf(stderr, "Port number must be between 1024 and 65535\n");
            return EXIT_FAILURE;
        }
    }

    printf("Port Number: %d\n", port);

    int sockfd;
    struct sockaddr_in serveradr;

    // create a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Socket Error\n");
        return EXIT_FAILURE;
    }

    // set up address
    bzero(&serveradr, sizeof(serveradr));
    serveradr.sin_family = AF_INET;
    serveradr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveradr.sin_port = htons(port);

    // bind listening socket to address
    if ((bind(sockfd, (struct sockaddr *)&serveradr, sizeof(serveradr))) < 0)
    {
        fprintf(stderr, "Bind Error\n");
        return EXIT_FAILURE;
    }

    // listen on the socket
    if ((listen(sockfd, 10)) < 0)
    {
        fprintf(stderr, "Listen Error\n");
        return EXIT_FAILURE;
    }

    // server loop
    int connection;
    char inbuf[4096];
    char outbuf[4096];
    int bytes;
    while (1)
    {
        struct sockaddr_in adr;
        socklen_t adrlen;
        printf("Server running\n");
        connection = accept(sockfd, (struct sockaddr *)NULL, NULL);
        for (int i = 0; i < 4096; i++)
        {
            inbuf[i] = 0;
        }
        while ((bytes = read(connection, inbuf, 4095)) > 0)
        {
            printf("%s", inbuf);
            // detect end of http request
            if (inbuf[bytes - 1] == '\n' && inbuf[bytes - 2] == '\r' && inbuf[bytes - 3] == '\n' && inbuf[bytes - 4] == '\r')
            {
                break;
            }
        }
        if (bytes == -1)
        {
            fprintf(stderr, "Read Error\n");
            return EXIT_FAILURE;
        }
        //single response for noe
        snprintf(outbuf, sizeof(outbuf), "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 25\r\n\r\n<---- contents here ---->" );
        write(connection, outbuf, strlen(outbuf));
        close(connection);
    }
    return EXIT_SUCCESS;
}
