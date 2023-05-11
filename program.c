#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_ADDRESS "10.0.3.15"
#define SERVER_PORT 67
#define DHCP_DISCOVERY 1

int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Error opening socket");
        exit(1);
    }

    // Bind the socket to the server address and port
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_ADDRESS);
    server_addr.sin_port = htons(SERVER_PORT);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    printf("DHCP server listening on %s:%d...\n", SERVER_ADDRESS, SERVER_PORT);

    // Listen for DHCP discovery messages
    while (1) {
        int len = sizeof(client_addr);
        int n = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, (unsigned int *)&len);
        if (n < 0) {
            perror("Error receiving message");
            exit(1);
        }

        // Check if message is a DHCP discovery message
        if (buffer[0] == DHCP_DISCOVERY) {
            printf("Received DHCP discovery message from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        }
    }

    return 0;
}
