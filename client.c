#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>

#define SERVER_PORT 12345
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

    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = 0;

    if (bind(sockfd, (struct sockaddr *) &client_addr, sizeof(client_addr)) < 0) {
        perror("Error on binding");
        exit(1);
    }

    printf("DHCP client started...\n");

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_BROADCAST;
    server_addr.sin_port = htons(SERVER_PORT);
    socklen_t server_len = sizeof(server_addr);
    srand(time(NULL));
    int xid = rand();
    sprintf(buffer, "DHCPDISCOVER %d", xid);

    int broadcast_enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Error in enabling broadcast");
        exit(1);
    }

    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error in sendto");
        exit(1);
    }

    printf("Send DHCP discover message to server...\n");

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error in setsockopt");
        exit(1);
    }

    int n = recvfrom(sockfd, buffer, MAX_MSG_SIZE, 0, (struct sockaddr *) &server_addr, &server_len);
    if (n < 0) {
        perror("Error in recvfrom");
        exit(1);
    }


    printf("Received DHCP offer from server %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));
    sprintf(buffer, "DHCPREQUEST %d", xid);
    if (sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("Error in second sendto");
        exit(1);
    }
    printf("Send DHCP request message to server...\n");
    int n2 = recvfrom(sockfd, buffer, MAX_MSG_SIZE, 0, (struct sockaddr *) &server_addr, &server_len);
    if (n2 < 0) {
        perror("Error in second recvfrom");
        exit(1);
    }
    printf("Received DHCP acknowledgement from server %s:%d\n", inet_ntoa(server_addr.sin_addr), ntohs(server_addr.sin_port));

    close(sockfd);
    return 0;
}