#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345
#define CLIENT_PORT 99
#define MAX_MSG_SIZE 1024


int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[MAX_MSG_SIZE];
    socklen_t server_len;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(CLIENT_PORT);

    if (bind(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0) {
        perror("Error on binding");
        exit(1);
    }

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(SERVER_PORT);
    server_len = sizeof(server_addr);
    // printf("Sending DHCP request to %s:%d\n", SERVER_IP, SERVER_PORT);
    
    strcpy(buffer, "DHCPDISCOVER");
    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error in sendto");
        exit(1);
    }

    int n = recvfrom(sockfd, buffer, MAX_MSG_SIZE, 0, (struct sockaddr *) &server_addr, &server_len);
    if (n < 0) {
        perror("Error in recvfrom");
        exit(1);
    }
    printf("Received DHCP offer from server %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    close(sockfd);
    return 0;
}