// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <arpa/inet.h>
// #include <unistd.h>
// #include <net/if.h>
// #include <netdb.h>

// #define DHCP_SERVER_PORT 12345
// #define DHCP_CLIENT_PORT 1234

// typedef struct {
//     uint8_t op;
//     uint8_t htype;
//     uint8_t hlen;
//     uint8_t hops;
//     uint32_t xid;
//     uint16_t secs;
//     uint16_t flags;
//     struct in_addr ciaddr;
//     struct in_addr yiaddr;
//     struct in_addr siaddr;
//     struct in_addr giaddr;
//     uint8_t chaddr[16];
//     uint8_t options[312];
// } dhcp_message;

// int main(int argc, char *argv[]) {
//     if (argc != 2) {
//         printf("Usage: %s <interface>\n", argv[0]);
//         return 1;
//     }

//     int sockfd;
//     struct sockaddr_in servaddr_send, servaddr_recv;
//     socklen_t servaddr_recv_len = sizeof(servaddr_recv);
//     struct ifreq ifr;
//     struct in_addr subnet_mask, router_ip, server_ip;
//     uint32_t lease_time, renewal_time, rebinding_time;
//     dhcp_message dhcp_discover, dhcp_offer, dhcp_request, dhcp_ack;
//     uint8_t *option_ptr;
//     ssize_t len;

//     // Create a socket for sending and receiving DHCP messages
//     sockfd = socket(AF_INET, SOCK_DGRAM, 0);
//     if (sockfd < 0) {
//         perror("socket");
//         return 1;
//     }

//     // Bind the socket to the client port
//     struct sockaddr_in client_addr = { 0 };
//     client_addr.sin_family = AF_INET;
//     client_addr.sin_port = htons(DHCP_CLIENT_PORT);
//     client_addr.sin_addr.s_addr = INADDR_ANY;
//     if (bind(sockfd, (struct sockaddr*) &client_addr, sizeof(client_addr)) < 0) {
//         perror("bind");
//         return 1;
//     }

//     // Set the interface for sending and receiving DHCP messages
//     strncpy(ifr.ifr_name, argv[1], IFNAMSIZ);

//     int broadcastEnable = 1;
//     setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

//     // if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
//     //     perror("setsockopt");
//     //     return 1;
//     // }

//     // Set up the server address for sending DHCP messages
//     memset(&servaddr_send, 0, sizeof(servaddr_send));
//     servaddr_send.sin_family = AF_INET;
//     servaddr_send.sin_port = htons(DHCP_SERVER_PORT);
//     inet_pton(AF_INET, "255.255.255.255", &servaddr_send.sin_addr);

//     // Set up the DHCP discover message
//     memset(&dhcp_discover, 0, sizeof(dhcp_message));
//     dhcp_discover.op = 1; // BOOTREQUEST
//     dhcp_discover.htype = 1; // Ethernet
//     dhcp_discover.hlen = 6; // MAC address length
//     dhcp_discover.xid = random();
//     dhcp_discover.flags = htons(0x8000); // Broadcast flag
//     memcpy(dhcp_discover.chaddr, &ifr.ifr_hwaddr.sa_data, 6);

//     option_ptr = dhcp_discover.options;
//     *option_ptr++ = 53; // DHCP message type option
//     *option_ptr++ = 1; // Length
//     *option_ptr++ = 1; // DHCP
// // Send the DHCP discover message
// printf("Sending DHCP discover message...\n");
// if (sendto(sockfd, &dhcp_discover, sizeof(dhcp_message), 0, (struct sockaddr*) &servaddr_send, sizeof(servaddr_send)) < 0) {
//     perror("sendto");
//     return 1;
// }

// // Wait for a DHCP offer message from the server
// printf("Waiting for DHCP offer message...\n");
// len = recvfrom(sockfd, &dhcp_offer, sizeof(dhcp_message), 0, (struct sockaddr*) &servaddr_recv, &servaddr_recv_len);
// if (len < 0) {
//     perror("recvfrom");
//     return 1;
// }

// // Parse the DHCP offer message
// printf("Received DHCP offer message.\n");
// if (dhcp_offer.op != 2 || dhcp_offer.htype != 1 || dhcp_offer.hlen != 6 || dhcp_offer.xid != dhcp_discover.xid) {
//     printf("Invalid DHCP offer message.\n");
//     return 1;
// }

// option_ptr = dhcp_offer.options;
// while (*option_ptr != 0xFF) {
//     if (*option_ptr == 1) { // Subnet mask
//         memcpy(&subnet_mask, option_ptr + 2, sizeof(subnet_mask));
//     } else if (*option_ptr == 3) { // Router IP
//         memcpy(&router_ip, option_ptr + 2, sizeof(router_ip));
//     } else if (*option_ptr == 51) { // Lease time
//         memcpy(&lease_time, option_ptr + 2, sizeof(lease_time));
//         lease_time = ntohl(lease_time);
//     } else if (*option_ptr == 58) { // Renewal time
//         memcpy(&renewal_time, option_ptr + 2, sizeof(renewal_time));
//         renewal_time = ntohl(renewal_time);
//     } else if (*option_ptr == 59) { // Rebinding time
//         memcpy(&rebinding_time, option_ptr + 2, sizeof(rebinding_time));
//         rebinding_time = ntohl(rebinding_time);
//     }

//     option_ptr += option_ptr[1] + 2;
// }

// // Print the DHCP lease information
// printf("DHCP lease information:\n");
// printf("IP address: %s\n", inet_ntoa(dhcp_ack.yiaddr));
// printf("Subnet mask: %s\n", inet_ntoa(subnet_mask));
// printf("Router IP: %s\n", inet_ntoa(router_ip));
// printf("Lease time: %u seconds\n", lease_time);
// printf("Renewal time: %u seconds\n", renewal_time);
// printf("Rebinding time: %u seconds\n", rebinding_time);

// // Close the socket
// close(sockfd);

// return 0;
// }

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
#define CLIENT_PORT 1234
#define MAXLINE 1024

struct dhcp_msg {
    uint8_t op, htype, hlen, hops;
    uint32_t xid;
    uint16_t secs, flags;
    uint32_t ciaddr, yiaddr, siaddr, giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t magic_cookie;
    uint8_t options[312];
};

int main(int argc, char *argv[]) {
    int sockfd;
    char buffer[MAXLINE];
    struct sockaddr_in servaddr_send, servaddr_recv;

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Setting SO_BROADCAST option on socket
    int broadcastEnable = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable));

    // Filling server information
    memset(&servaddr_send, 0, sizeof(servaddr_send));
    memset(&servaddr_recv, 0, sizeof(servaddr_recv));

    servaddr_send.sin_family = AF_INET;
    servaddr_send.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, "0.0.0.0", &servaddr_send.sin_addr);

    servaddr_recv.sin_family = AF_INET;
    servaddr_recv.sin_port = htons(CLIENT_PORT);
    inet_pton(AF_INET, "0.0.0.0", &servaddr_recv.sin_addr);

    // Binding the socket with the client address
    if (bind(sockfd, (const struct sockaddr *)&servaddr_recv, sizeof(servaddr_recv)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    struct dhcp_msg dhcp_message;
    memset(&dhcp_message, 0, sizeof(dhcp_message));

    dhcp_message.op = 1;
    dhcp_message.htype = 1;
    dhcp_message.hlen = 6;
    dhcp_message.hops = 0;

    dhcp_message.xid = htonl(rand());
    dhcp_message.secs = htons(0);
    dhcp_message.flags = htons(0x8000);

    dhcp_message.chaddr[0] = 0x00;
    dhcp_message.chaddr[1] = 0x0c;
    dhcp_message.chaddr[2] = 0x29;
    dhcp_message.chaddr[3] = 0x9d;
    dhcp_message.chaddr[4] = 0x1d;
    dhcp_message.chaddr[5] = 0xa9;

    dhcp_message.magic_cookie = htonl(0x63825363);

    dhcp_message.options[0] = 53; // Option 53 (DHCP Message Type)
    dhcp_message.options[1] = 1; // Option length
    dhcp_message.options[2] = 1; // DHCPDISCOVER

    dhcp_message.options[3] = 255; // End of options
    // Sending the DHCP Discover message
    int n;
    socklen_t len;
    sendto(sockfd, &dhcp_message, sizeof(dhcp_message), 0, (const struct sockaddr *)&servaddr_send, sizeof(servaddr_send));

    // Receiving the DHCP Offer message
    n = recvfrom(sockfd, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&servaddr_recv, &len);
    buffer[n] = '\0';

    // Parsing the DHCP Offer message
    struct dhcp_msg dhcp_offer;
    memset(&dhcp_offer, 0, sizeof(dhcp_offer));
    memcpy(&dhcp_offer, buffer, sizeof(dhcp_offer));

    uint32_t offered_ip = dhcp_offer.yiaddr;
    uint32_t subnet_mask = 0;
    uint32_t router_ip = 0;
    uint32_t lease_time = 0;
    uint32_t renewal_time = 0;
    uint32_t rebinding_time = 0;

    // Extracting subnet mask, router IP, lease time, renewal time, and rebinding time from DHCP Offer options
    uint8_t *options_ptr = dhcp_offer.options;
    while (*options_ptr != 255) {
        if (*options_ptr == 1) { // Subnet Mask option
            subnet_mask = *(uint32_t *)(options_ptr + 2);
        } else if (*options_ptr == 3) { // Router option
            router_ip = *(uint32_t *)(options_ptr + 2);
        } else if (*options_ptr == 51) { // Lease Time option
            lease_time = ntohl(*(uint32_t *)(options_ptr + 2));
        } else if (*options_ptr == 58) { // Renewal Time option
            renewal_time = ntohl(*(uint32_t *)(options_ptr + 2));
        } else if (*options_ptr == 59) { // Rebinding Time option
            rebinding_time = ntohl(*(uint32_t *)(options_ptr + 2));
        }
        options_ptr += options_ptr[1] + 2;
    }

    // Printing the offered IP address, subnet mask, router IP, lease time, renewal time, and rebinding time
    printf("Offered IP address: %s\n", inet_ntoa(*(struct in_addr *)&offered_ip));
    printf("Subnet mask: %s\n", inet_ntoa(*(struct in_addr *)&subnet_mask));
    printf("Router IP: %s\n", inet_ntoa(*(struct in_addr *)&router_ip));
    printf("Lease time: %d seconds\n", lease_time);
    printf("Renewal time: %d seconds\n", renewal_time);
    printf("Rebinding time: %d seconds\n", rebinding_time);

return 0;
}
