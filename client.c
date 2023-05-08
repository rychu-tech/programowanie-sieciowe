#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_MAGIC_COOKIE 0x63825363
#define DHCP_DISCOVER_MSG_TYPE 0x01
#define SERVER_ADDR "255.255.255.255"

typedef struct {
    uint8_t opcode;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    struct in_addr ciaddr;
    struct in_addr yiaddr;
    struct in_addr siaddr;
    struct in_addr giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint32_t cookie;
} dhcp_packet;

void print_offer_packet(dhcp_packet* packet) {
    printf("=== DHCP Offer ===\n");
    printf("Opcode: 0x%02X\n", packet->opcode);
    printf("Hardware Type: 0x%02X\n", packet->htype);
    printf("Hardware Length: 0x%02X\n", packet->hlen);
    printf("Hops: 0x%02X\n", packet->hops);
    printf("Transaction ID: 0x%08X\n", packet->xid);
    printf("Seconds Elapsed: 0x%04X\n", packet->secs);
    printf("Flags: 0x%04X\n", packet->flags);
    printf("Client IP Address: %s\n", inet_ntoa(packet->ciaddr));
    printf("Your IP Address: %s\n", inet_ntoa(packet->yiaddr));
    printf("Server IP Address: %s\n", inet_ntoa(packet->siaddr));
    printf("Gateway IP Address: %s\n", inet_ntoa(packet->giaddr));
    printf("Client Hardware Address: ");
    for (int i = 0; i < packet->hlen; i++) {
        printf("%02X:", packet->chaddr[i]);
    }
    printf("\b \n"); // backspace to remove last ':'
    printf("Server Host Name: %s\n", packet->sname);
    printf("Boot File Name: %s\n", packet->file);
    printf("Magic Cookie: 0x%08X\n", packet->cookie);
    return;
}

int main(int argc, char **argv) {
    int sockfd;
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    struct ifreq ifr;
    strcpy(ifr.ifr_name, argv[1]);
    if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("ioctl failed");
        exit(EXIT_FAILURE);
    }
    unsigned char *macaddr = (unsigned char *)ifr.ifr_hwaddr.sa_data;


    struct sockaddr_in serveraddr;
    dhcp_packet discover_packet;
    dhcp_packet offer_packet;
    memset(&discover_packet, 0, sizeof(discover_packet));

    discover_packet.opcode = 0x01; // BOOTREQUEST
    discover_packet.htype = 0x01; // Ethernet
    discover_packet.hlen = 0x06; // Hardware address length
    discover_packet.hops = 0x00; // Hops
    discover_packet.xid = rand(); // Transaction ID
    discover_packet.secs = 0x0000; // Seconds elapsed
    discover_packet.flags = htons(0x8000); // Broadcast flag
    discover_packet.ciaddr.s_addr = 0; // Client IP address (0.0.0.0)
    discover_packet.yiaddr.s_addr = 0; // Your IP address (0.0.0.0)
    discover_packet.siaddr.s_addr = 0; // Server IP address (0.0.0.0)
    discover_packet.giaddr.s_addr = 0; // Gateway IP address (0.0.0.0)
    memset(discover_packet.chaddr, 0, sizeof(discover_packet.chaddr)); // Client hardware address
    memcpy(discover_packet.chaddr, macaddr, 6); // Replace with your own MAC address
    memset(discover_packet.sname, 0, sizeof(discover_packet.sname)); // Server host name
    memset(discover_packet.file, 0, sizeof(discover_packet.file)); // Boot file name
    discover_packet.cookie = htonl(DHCP_MAGIC_COOKIE); // Magic cookie

    // Option 53: DHCP Message Type
    uint8_t dhcp_msg_type[3] = {0x35, 0x01, DHCP_DISCOVER_MSG_TYPE};
    // Option 55: Parameter Request List
    uint8_t dhcp_param_req_list[9] = {0x37, 0x09, 0x01, 0x03, 0x06, 0x0f, 0x1f, 0x21, 0xff};
    // Option 255: End
    uint8_t dhcp_end[1] = {0xff};

    char buffer[1024];

    

    // Enable broadcast
    int broadcastEnable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcastEnable, sizeof(broadcastEnable)) < 0) {
        perror("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    serveraddr.sin_family = AF_INET;
       serveraddr.sin_port = htons(DHCP_SERVER_PORT);
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR); // Broadcast address

    // Construct the DHCP message
    memcpy(buffer, &discover_packet, sizeof(dhcp_packet));
    memcpy(buffer + sizeof(dhcp_packet), dhcp_msg_type, sizeof(dhcp_msg_type));
    memcpy(buffer + sizeof(dhcp_packet) + sizeof(dhcp_msg_type), dhcp_param_req_list, sizeof(dhcp_param_req_list));
    memcpy(buffer + sizeof(dhcp_packet) + sizeof(dhcp_msg_type) + sizeof(dhcp_param_req_list), dhcp_end, sizeof(dhcp_end));
    int packet_size = sizeof(dhcp_packet) + sizeof(dhcp_msg_type) + sizeof(dhcp_param_req_list) + sizeof(dhcp_end);

    // Send the DHCP discover message
    if (sendto(sockfd, buffer, packet_size, 0, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("sendto");
        exit(1);
    }

    printf("DHCP Discover sent to %s\n", inet_ntoa(serveraddr.sin_addr));

    int n;
    socklen_t len;
    n = recvfrom(sockfd, &offer_packet, sizeof(offer_packet), 0, (struct sockaddr *)&serveraddr, &len);
    if (n < 0) {
        perror("recvfrom failed");
        exit(EXIT_FAILURE);
    }

    printf("DHCP Offer received from %s\n", inet_ntoa(serveraddr.sin_addr));
    print_offer_packet(&offer_packet);
    close(sockfd);

    return 0;
}

