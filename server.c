#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_PORT 12345
#define CLIENT_PORT 99
#define MAX_MSG_SIZE 1024

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAX_MSG_SIZE];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error on binding");
        exit(1);
    }
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getsockname(sockfd, (struct sockaddr *)&addr, &len);
    printf("DHCP server started on port %d\n", ntohs(addr.sin_port));
    while (1) {
        socklen_t client_len = sizeof(client_addr);
        int n = recvfrom(sockfd, buffer, MAX_MSG_SIZE, 0, (struct sockaddr *) &client_addr, &client_len);
        if (n < 0) {
            perror("Error in recvfrom");
            exit(1);
        }
        printf("Received DHCP request from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        strcpy(buffer, "DHCPOFFER");
        n = sendto(sockfd, buffer, n, 0, (struct sockaddr *) &client_addr, client_len);
        if (n < 0) {
            perror("Error in sendto");
            exit(1);
        }
    }
    close(sockfd);
    return 0;
}