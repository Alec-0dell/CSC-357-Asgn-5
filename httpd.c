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
    struct sockaddr_in serveraddr;
    // int buf[4097];

    // create a socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Socket Error\n");
        return EXIT_FAILURE;
    }

    // set up address
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    // bind listening socket to address
    if ((bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr))) < 0)
    {
        fprintf(stderr, "Bind Error\n");
        return EXIT_FAILURE;
    }

    //listen on the socket
    if ((listen(sockfd, 10)) < 0)
    {
        fprintf(stderr, "Listen Error\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
