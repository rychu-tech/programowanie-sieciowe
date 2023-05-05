#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DHCP_SERVER_PORT 12345
#define DHCP_CLIENT_PORT 1234

typedef struct {
    uint8_t op;         // Message opcode
    uint8_t htype;      // Hardware address type
    uint8_t hlen;       // Hardware address length
    uint8_t hops;       // Number of relay agent hops
    uint32_t xid;       // Transaction ID
    uint16_t secs;      // Seconds since boot
    uint16_t flags;     // Flags
    uint32_t ciaddr;    // Client IP address
    uint32_t yiaddr;    // Your (client) IP address
    uint32_t siaddr;    // Server IP address
    uint32_t giaddr;    // Relay agent IP address
    uint8_t chaddr[16]; // Client hardware address
    uint8_t padding[192];
    uint32_t magic_cookie; // Magic cookie
    uint8_t options[64];   // Options
} dhcp_message;

int main(int argc, char* argv[]) {
    int sockfd, broadcast = 1;
    struct sockaddr_in servaddr, cliaddr;
    dhcp_message dhcp_offer;
    uint8_t mac_addr[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55}; // Change this to your own MAC address
    uint32_t ip_addr = htonl(0xc0a80101); // Change this to your own IP address
    uint32_t netmask = htonl(0xffffff00); // Change this to your network mask
    uint32_t router_ip = htonl(0xc0a80101); // Change this to your router's IP address
    uint32_t dns_ip = htonl(0xc0a80101); // Change this to your DNS server's IP address
    uint32_t lease_time = htonl(86400); // Change this to the lease time in seconds

    // Create a UDP socket
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Set the socket to allow broadcast
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the DHCP server port
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(DHCP_SERVER_PORT);

    if (bind(sockfd, (struct sockaddr*) &servaddr, sizeof(servaddr)) < 0) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    uint32_t ip_pool[] = {
        htonl(0xc0a80102), // 192.168.1.2
        htonl(0xc0a80103), // 192.168.1.3
        htonl(0xc0a80104), // 192.168.1.4
        htonl(0xc0a80105)  // 192.168.1.5
    };
    int poolInx = 0;
    int numPoolIps = sizeof(ip_pool) / sizeof(ip_pool[0]);

    while (1) {
        // Receive a DHCP discover message from a client
        memset(&dhcp_offer, 0, sizeof(dhcp_message));
        socklen_t len = sizeof(cliaddr);
        if (recvfrom(sockfd, &dhcp_offer, sizeof(dhcp_message), 0, (struct sockaddr*) &cliaddr, &len) < 0) {
            perror("recvfrom");
        continue;
    }

    // Check that the message is a DHCP discover message
    if (dhcp_offer.op != 1 || dhcp_offer.htype != 1 || dhcp_offer.hlen != 6 || dhcp_offer.magic_cookie != htonl(0x63825363)) {
        continue;
    }

    if (poolInx >= numPoolIps) {
        printf("No more available addresses.\n");
        continue;
        
    }

    // Fill in the DHCP offer message
    memset(&dhcp_offer, 0, sizeof(dhcp_message));
    dhcp_offer.op = 2; // Boot reply
    dhcp_offer.htype = 1; // Ethernet
    dhcp_offer.hlen = 6; // Ethernet MAC address length
    dhcp_offer.xid = rand(); // Transaction ID
    dhcp_offer.yiaddr = ip_pool[poolInx]; // Offered IP address
    dhcp_offer.siaddr = htonl(ip_addr); // Server IP address
    dhcp_offer.magic_cookie = htonl(0x63825363); // Magic cookie

    // Add DHCP options
    uint8_t* option_ptr = dhcp_offer.options;
    *option_ptr++ = 53; // DHCP message type
    *option_ptr++ = 1;
    *option_ptr++ = 2; // DHCP offer
    *option_ptr++ = 1; // Subnet mask
    *option_ptr++ = 4;
    memcpy(option_ptr, &netmask, sizeof(netmask));
    option_ptr += 4;
    *option_ptr++ = 3; // Router option
    *option_ptr++ = 4;
    memcpy(option_ptr, &router_ip, sizeof(router_ip));
    option_ptr += 4;
    *option_ptr++ = 6; // DNS server option
    *option_ptr++ = 4;
    memcpy(option_ptr, &dns_ip, sizeof(dns_ip));
    option_ptr += 4;
    *option_ptr++ = 51; // IP address lease time
    *option_ptr++ = 4;
    memcpy(option_ptr, &lease_time, sizeof(lease_time));
    option_ptr += 4;
    *option_ptr++ = 255; // End option

    // Send the DHCP offer message to the client
    if (sendto(sockfd, &dhcp_offer, sizeof(dhcp_message), 0, (struct sockaddr*) &cliaddr, sizeof(cliaddr)) < 0) {
        perror("sendto");
        continue;
    }
    poolInx++;
}

return 0;
}
