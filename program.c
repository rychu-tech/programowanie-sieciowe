#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_DISCOVERY 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_ACK 5
#define DHCP_IP_RANGE_START "192.168.1.3"
#define DHCP_SERVER_IP "192.168.1.1"


typedef struct {
    uint8_t op;
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
    char sname[64];
    char file[128];
    uint32_t cookie;
    uint8_t options[308];
} dhcp_packet;


void print_mac_address(const unsigned char* mac_address) {
    printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
}

int create_offer_packet(dhcp_packet *offer_packet, dhcp_packet *discovery_packet) {
    memset(offer_packet, 0, sizeof(dhcp_packet));
    offer_packet->op = 2; // Boot reply
    offer_packet->htype = 1; // Ethernet
    offer_packet->hlen = 6; // MAC Address length
    offer_packet->hops = 0; // No relay agents
    offer_packet->xid = discovery_packet->xid;
    printf("%d\n", discovery_packet->xid);
    offer_packet->secs = 0; // No seconds elapsed
    offer_packet->flags = htons(0x8000); // Broadcast flag
    offer_packet->yiaddr.s_addr = inet_addr(DHCP_IP_RANGE_START); // Offered IP address
    offer_packet->siaddr.s_addr = inet_addr(DHCP_SERVER_IP); // Server address 
    offer_packet->ciaddr.s_addr = 0;
    offer_packet->giaddr.s_addr = 0;
    memcpy(offer_packet->chaddr, discovery_packet->chaddr, 6); // Copy client MAC
    memset(&offer_packet[6], 0, 10);
    memset(offer_packet->file, 0, sizeof(offer_packet->file));
    memset(offer_packet->sname, 0, sizeof(offer_packet->sname));
    offer_packet->cookie = htonl(0x63825363);

    print_mac_address(offer_packet->chaddr);
    int offset = 0;
    offer_packet->options[0] = 53; // DHCP message type option
    offer_packet->options[1] = 1; // Length of the option data
    offer_packet->options[2] = 2; // DHCP offer message type

    offer_packet->options[3] = 1; // Subnet mask option
    offer_packet->options[4] = 4; // Length of the option data
    offer_packet->options[5] = 255; //
    offer_packet->options[6] = 255; //
    offer_packet->options[7] = 255; //
    offer_packet->options[8] = 0; //

    offer_packet->options[9] = 3; // Router options
    offer_packet->options[10] = 4; // Option data length
    offer_packet->options[11] = 192; //
    offer_packet->options[12] = 168; //
    offer_packet->options[13] = 1; //
    offer_packet->options[14] = 1; //

    // offer_packet->options[15] = 51; // Ip address lease time option
    // offer_packet->options[16] = 4; // Option data length
    // offer_packet->options[17] = 0;
    // offer_packet->options[18] = 0;
    // offer_packet->options[19] = 1;
    // offer_packet->options[20] = 1; // Lease time in seconds

    // offer_packet->options[21] = 6; // Ip address lease time option
    // offer_packet->options[22] = 4; // Option data length
    // offer_packet->options[23] = 192;
    // offer_packet->options[24] = 160;
    // offer_packet->options[25] = 1;
    // offer_packet->options[26] = 1; // Lease time in seconds

    offer_packet->options[15] = 54; // Dhcp server identity option
    offer_packet->options[16] = 4; // Option data length
    offer_packet->options[17] = 192;
    offer_packet->options[18] = 168;
    offer_packet->options[19] = 1;
    offer_packet->options[20] = 1; // Lease time in seconds

    offer_packet->options[21] = 255; // End of options marker
    return sizeof(dhcp_packet);
}

int create_ack_packet(dhcp_packet *ack_packet, dhcp_packet *request_packet) {
    memset(ack_packet, 0, sizeof(dhcp_packet));
    ack_packet->op = 2; // Boot reply
    ack_packet->htype = 1; // Ethernet
    ack_packet->hlen = 6; // MAC Address length
    ack_packet->hops = 0; // No relay agents
    ack_packet->xid = request_packet->xid;
    ack_packet->secs = 0; // No seconds elapsed
    ack_packet->flags = htons(0x8000); // Broadcast flag
    ack_packet->yiaddr.s_addr = inet_addr(DHCP_IP_RANGE_START); // Assigned IP address
    ack_packet->siaddr.s_addr = inet_addr(DHCP_SERVER_IP); // Server address 
    ack_packet->ciaddr.s_addr = 0;
    ack_packet->giaddr.s_addr = 0;
    memcpy(ack_packet->chaddr, request_packet->chaddr, 6); // Copy client MAC
    memset(&ack_packet[6], 0, 10);
    memset(ack_packet->file, 0, sizeof(ack_packet->file));
    memset(ack_packet->sname, 0, sizeof(ack_packet->sname));
    ack_packet->cookie = htonl(0x63825363);

    print_mac_address(ack_packet->chaddr);
    int offset = 0;
    ack_packet->options[0] = 53; // DHCP message type option
    ack_packet->options[1] = 1; // Length of the option data
    ack_packet->options[2] = 5; // DHCP ACK message type

    ack_packet->options[3] = 1; // Subnet mask option
    ack_packet->options[4] = 4; // Length of the option data
    ack_packet->options[5] = 255; //
    ack_packet->options[6] = 255; //
    ack_packet->options[7] = 255; //
    ack_packet->options[8] = 0; //

    ack_packet->options[9] = 3; // Router options
    ack_packet->options[10] = 4; // Option data length
    ack_packet->options[11] = 192; //
    ack_packet->options[12] = 168; //
    ack_packet->options[13] = 1; //
    ack_packet->options[14] = 1; //

    ack_packet->options[15] = 51; // Ip address lease time option
    ack_packet->options[16] = 4; // Option data length
    ack_packet->options[17] = 0;
    ack_packet->options[18] = 14;
    ack_packet->options[19] = 1;
    ack_packet->options[20] = 0;

    ack_packet->options[21] = 54; // Dhcp server identity option
    ack_packet->options[22] = 4; // Option data length
    ack_packet->options[23] = 192;
    ack_packet->options[24] = 168;
    ack_packet->options[25] = 1;
    ack_packet->options[26] = 1;
    ack_packet->options[27] = 255; // End of options
    return sizeof(dhcp_packet);
}


int main() {
    int sockfd;
    struct sockaddr_in server_addr, client_addr;
    char buffer[1024];
    dhcp_packet discovery_packet, offer_packet, ack_packet;
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &enable, sizeof(enable)) < 0) {
        perror("Error setting socket enable");
        exit(1);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);
    bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    


    while(1) {
        printf("Waiting for client request...\n");
        int len = sizeof(client_addr);
        int n = recvfrom(sockfd, &discovery_packet, sizeof(dhcp_packet), 0, (struct sockaddr *)&client_addr, &len);
        printf("Received %d bytes from client\n", n);

        memset(&client_addr,0,sizeof(client_addr));
        client_addr.sin_family = AF_INET;
        client_addr.sin_port = htons(DHCP_CLIENT_PORT);
        client_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

        if (discovery_packet.options[2] == DHCP_DISCOVERY) {
            printf("DHCP Discovery message received from client\n");
            int offer_packet_size = create_offer_packet(&offer_packet, &discovery_packet);
            sendto(sockfd, &offer_packet, offer_packet_size, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            printf("Sending DHCP offer packet to client\n");
        }

        if (discovery_packet.options[2] == DHCP_REQUEST) {
            printf("DHCP Request message received from client\n");
            int ack_packet_size = create_ack_packet(&ack_packet, &discovery_packet);
            sendto(sockfd, &ack_packet, ack_packet_size, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            printf("Sending DHCP offer packet to client\n");
        }
    }

    return 0;
}
