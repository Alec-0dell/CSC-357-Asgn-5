#include "httpd.h"

#define BAD_REQUEST "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n400 bad request\n%s"
#define NOT_IMPLEMENTED "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/html\r\n\r\n501 Not Implemented\n%s"
#define DELAY_SUCCESS "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: 25\r\n\r\nResponse after a delay"
#define NOT_FOUND "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n404 Not found\n%s"
#define INTERNAL_ERROR "HTTP/1.1 500 Internal Error\r\nContent-Type: text/html\r\n\r\n500 Internal Error\n%s"
#define SUCCESS "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %lld\r\n\r\n"

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

    while (1)
    {
        printf("Server running\n");
        connection = accept(sockfd, (struct sockaddr *)NULL, NULL);
        pthread_t t;
        int *pclient = malloc(sizeof(int));
        *pclient = connection;
        pthread_create(&t, NULL, handle_connection, pclient);
    }
    return EXIT_SUCCESS;
}

void *handle_connection(void *pclient)
{
    int connection = *(int *)pclient;
    free(pclient);
    char inbuf[4096];
    char outbuf[4096];
    char path[PATH_MAX];
    char resolved_path[PATH_MAX];
    int bytes;
    char request[5];
    char delay[8];
    int comlen;
    memset(inbuf, 0, sizeof(inbuf));
    memset(outbuf, 0, sizeof(outbuf));
    memset(request, 0, sizeof(request));
    memset(delay, 0, sizeof(delay));
    while ((bytes = read(connection, inbuf, 4095)) > 0)
    {
        printf("%s", inbuf);
        // detect end of http request
        if (inbuf[bytes - 1] == '\n' && inbuf[bytes - 2] == '\r' && inbuf[bytes - 3] == '\n' && inbuf[bytes - 4] == '\r')
        {
            break;
        }
        else
        {
            printf("Error HTTP bad request\n");
            snprintf(outbuf, sizeof(outbuf), BAD_REQUEST, inbuf);
            write(connection, outbuf, strlen(outbuf));
            close(connection);
            return NULL;
        }
    }
    if (bytes == -1)
    {
        fprintf(stderr, "Read Error\n");
        return NULL;
    }

    // get or head request
    strncpy(request, inbuf, 4);
    printf("Request: |%s|\n", request);
    if (strcmp(request, "GET ") == 0)
    {
        printf("Get\n");
        comlen = 3;
    }
    else if (strcmp(request, "HEAD") == 0)
    {
        printf("Head\n");
        comlen = 4;
    }
    else
    {
        printf("501\n");
        snprintf(outbuf, sizeof(outbuf), NOT_IMPLEMENTED, inbuf);
        write(connection, outbuf, strlen(outbuf));
        close(connection);
        return NULL;
    }

    // delay request
    strncpy(delay, &(inbuf[comlen + 1]), 7);
    if (strcmp(delay, "/delay/") == 0)
    {
        printf("Delay\n");
        sleep(atoi(&inbuf[comlen + 8]));
        snprintf(outbuf, sizeof(outbuf), DELAY_SUCCESS);
        write(connection, outbuf, strlen(outbuf));
        close(connection);
        return NULL;
    }
    else
    {
        printf("No delay\n");
    }

    for (size_t i = comlen + 1; inbuf[i] != ' '; i++)
    {
        if (inbuf[i] == '.' && inbuf[i + 1] == '.')
        {
            printf("Error Directory Traversal\n");
            snprintf(outbuf, sizeof(outbuf), NOT_FOUND, inbuf);
            write(connection, outbuf, strlen(outbuf));
            close(connection);
            return NULL;
        }
    }

    // check if the file exists
    strcpy(path, &(inbuf[comlen + 2]));
    char *space = strchr(path, ' ');
    if (space)
        *space = '\0';
    if (realpath(path, resolved_path) == NULL)
    {
        printf("No path found: |%s| %d\n", resolved_path, comlen);
        snprintf(outbuf, sizeof(outbuf), NOT_FOUND, inbuf);
        write(connection, outbuf, strlen(outbuf));
        close(connection);
        return NULL;
    }

    // open the file
    FILE *fp = fopen(resolved_path, "r");
    if (fp == NULL)
    {
        printf("Error opening\n");
        snprintf(outbuf, sizeof(outbuf), INTERNAL_ERROR, inbuf);
        write(connection, outbuf, strlen(outbuf));
        close(connection);
        return NULL;
    }

    struct stat statbuf;
    if (stat(resolved_path, &statbuf) == -1)
    {
        fprintf(stderr, "Read Error\n");
        fclose(fp);
        close(connection);
        return NULL;
    }

    snprintf(outbuf, sizeof(outbuf), SUCCESS, (long long)statbuf.st_size);
    write(connection, outbuf, strlen(outbuf));

    // Send file contents
    if (comlen == 3)
    {
        while ((bytes = fread(outbuf, 1, sizeof(outbuf), fp)) > 0)
        {
            write(connection, outbuf, bytes);
        }
    }

    fclose(fp);
    close(connection);
    return NULL;
}
