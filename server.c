#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_MAGIC_COOKIE 0x63825363
#define DHCP_DISCOVER_MSG_TYPE 0x01
#define DHCP_OFFER_MSG_TYPE 0x02
#define DHCP_NETMASK_OPTION 0x01
#define DHCP_DNS_OPTION 0x06
#define DHCP_SERVER_ID_OPTION 0x36
#define DHCP_LEASE_TIME_OPTION 0x33
#define SERVER_ADDR "0.0.0.0"

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

typedef struct {
    uint8_t option;
    uint8_t length;
    uint8_t value[256];
} dhcp_option;

void send_dhcp_offer(int sockfd, struct sockaddr_in clientaddr, dhcp_packet discover_packet) {
    dhcp_packet offer_packet;

    memset(&offer_packet, 0, sizeof(offer_packet));

    offer_packet.opcode = 0x02; // BOOTREPLY
    offer_packet.htype = 0x01; // Ethernet
    offer_packet.hlen = 0x06; // Hardware address length
    offer_packet.hops = 0x00; // Hops
    offer_packet.xid = discover_packet.xid; // Transaction ID
    offer_packet.secs = 0x0000; // Seconds elapsed
    offer_packet.flags = discover_packet.flags; // Unicast flag
    offer_packet.ciaddr.s_addr = 0; // Client IP address (0.0.0.0)
    offer_packet.yiaddr.s_addr = inet_addr("192.168.1.10"); // Offered IP address
    offer_packet.siaddr.s_addr = inet_addr(SERVER_ADDR); // Server IP address
    offer_packet.giaddr.s_addr = discover_packet.giaddr.s_addr; // Gateway IP address
    memcpy(offer_packet.chaddr, discover_packet.chaddr, sizeof(offer_packet.chaddr)); // Client hardware address
    // memset(offer_packet.sname, 0, sizeof(offer_packet.sname)); // Server host name
    // memset(offer_packet.file, 0, sizeof(offer_packet.file)); // Boot file name
    offer_packet.cookie = htonl(DHCP_MAGIC_COOKIE); // Magic cookie

    // Option 53: DHCP Message Type
    dhcp_option dhcp_msg_type = {0x35, 0x01, DHCP_OFFER_MSG_TYPE};
    // Option 1: Subnet Mask
    uint8_t dhcp_netmask[6] = {0x01, 0x04, 0xff, 0xff, 0xff, 0x00}; // 255.255.255.0
    // Option 6: DNS Server
    uint8_t dhcp_dns[6] = {0x06, 0x04, 0xc0, 0xa8, 0x01, 0x01}; // 192.168.1.1
    // Option 51: IP Address Lease Time
    uint8_t dhcp_lease_time[6] = {0x33, 0x04, 0x00, 0x01, 0x51, 0x80}; // 86400 seconds (1 day)
    memcpy(&offer_packet.cookie + sizeof(offer_packet.cookie), &dhcp_msg_type, sizeof(dhcp_option));
    memcpy(&offer_packet.cookie + sizeof(offer_packet.cookie) + sizeof(dhcp_option), dhcp_netmask, sizeof(dhcp_netmask));
    memcpy(&offer_packet.cookie + sizeof(offer_packet.cookie) + sizeof(dhcp_option) + sizeof(dhcp_netmask), dhcp_dns, sizeof(dhcp_dns));

    // Send DHCP offer packet
    sendto(sockfd, &offer_packet, sizeof(offer_packet), 0, (struct sockaddr*)&clientaddr, sizeof(clientaddr));
}
int main(int argc, char *argv[]) {
    int sockfd;
    struct sockaddr_in serveraddr, clientaddr;
    dhcp_packet discover_packet;
    sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(DHCP_SERVER_PORT);
    serveraddr.sin_addr.s_addr = inet_addr(SERVER_ADDR);

    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("bind");
        exit(1);
    }

    printf("Listening for DHCP requests...\n");

    while (1) {
        memset(&discover_packet, 0, sizeof(discover_packet));

        int n = recvfrom(sockfd, &discover_packet, sizeof(discover_packet), 0, (struct sockaddr *)&clientaddr, &(socklen_t){sizeof(clientaddr)});
        if (n < 0) {
            perror("recvfrom");
            exit(1);
        }

        if (discover_packet.opcode == 0x01 && discover_packet.htype == 0x01 && discover_packet.hlen == 0x06 && discover_packet.cookie == htonl(DHCP_MAGIC_COOKIE)) {
            printf("Received DHCP Discover message from %s\n", inet_ntoa(clientaddr.sin_addr));

            send_dhcp_offer(sockfd, clientaddr, discover_packet);

            printf("Sent DHCP Offer message to %s\n", inet_ntoa(clientaddr.sin_addr));
        }
    }

    return 0;
}