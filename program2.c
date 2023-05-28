#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define SERVER_PORT 67
#define DHCP_CLIENT_PORT 68
#define DHCP_DISCOVERY 1
#define DHCP_OFFER 2
#define DHCP_REQUEST 3
#define DHCP_ACK 5

#define DHCP_IP_RANGE_START "192.168.1.2"
#define DHCP_IP_RANGE_END "192.168.1.255"
#define IP_POOL_SIZE 256
#define SUBNET "255.255.255.0"
#define INTERFACE "enp0s3"
#define LEASE_TIME 86400 // 24 hours
#define RENEWAL_TIME 43200 // 12 hours


typedef enum {
    FREE,
    TAKEN,
    IN_PROGRESS
} ip_status;

typedef struct {
    uint8_t octets[4];
    ip_status status;
    time_t *timestamp;
    const char *mac_address;
} ip_address;

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

#define MAX_INTERFACE_LEN 16

char* getInterfaceIP(const char* interface) {
    int sockfd;
    struct ifreq ifr;
    struct sockaddr_in* addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(1);
    }

    strncpy(ifr.ifr_name, interface, IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';

    if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1) {
        perror("ioctl");
        close(sockfd);
        exit(1);
    }

    addr = (struct sockaddr_in*)&ifr.ifr_addr;
    char* ip = strdup(inet_ntoa(addr->sin_addr));

    close(sockfd);
    return ip;
}

void parseStringToOctets(const char* value, uint8_t octets[4]) {
    sscanf(value, "%hhu.%hhu.%hhu.%hhu", &octets[0], &octets[1], &octets[2], &octets[3]);
}

void initialize_ip_pool(ip_address* pool) {
    uint8_t startOctets[4] = {0};
    uint8_t endOctets[4] = {0};

    sscanf(DHCP_IP_RANGE_START, "%hhu.%hhu.%hhu.%hhu", &startOctets[0], &startOctets[1], &startOctets[2], &startOctets[3]);
    sscanf(DHCP_IP_RANGE_END, "%hhu.%hhu.%hhu.%hhu", &endOctets[0], &endOctets[1], &endOctets[2], &endOctets[3]);

    int diff = (endOctets[0] - startOctets[0]) * 256 * 256 * 256 +
               (endOctets[1] - startOctets[1]) * 256 * 256 +
               (endOctets[2] - startOctets[2]) * 256 +
               (endOctets[3] - startOctets[3]);

    if (diff < 0) {
        printf("Error: End IP should be greater than or equal to Start IP.\n");
        return;
    }

    for (int i = 0; i < IP_POOL_SIZE; i++) {
        pool[i].octets[0] = startOctets[0] + (i / (256 * 256 * 256));
        pool[i].octets[1] = startOctets[1] + ((i / (256 * 256)) % 256);
        pool[i].octets[2] = startOctets[2] + ((i / 256) % 256);
        pool[i].octets[3] = startOctets[3] + (i % 256);
        pool[i].status = FREE;
        pool[i].timestamp = NULL;
        pool[i].mac_address = NULL;
    }
}



void print_ip_address(ip_address addr) {
    printf("%d.%d.%d.%d\n", addr.octets[0],addr.octets[1],addr.octets[2],addr.octets[3]);
}

void print_ip_pool(ip_address* pool) {
    for (int i = 0; i < IP_POOL_SIZE - 2; i++) {
        printf("IP Address: ");
        print_ip_address(pool[i]);
        printf(" Status: %d", pool[i].status);
        if (pool[i].timestamp != NULL) {
            printf(" Timestamp: %ld", *pool[i].timestamp);
        }
        printf("\n");
    }
}

void clean_ip_pool(ip_address* pool) {
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (pool[i].timestamp != NULL) {
            if (*pool[i].timestamp + RENEWAL_TIME <= time(NULL)) {
                pool[i].timestamp = NULL;
                pool[i].status = FREE;
                pool[i].mac_address = NULL;
                printf("IP POOL CLEANED UP");
            }
        }
    }
}


void print_mac_address(const unsigned char* mac_address) {
    printf("MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5]);
}

ip_address* get_free_ip(ip_address* pool, const uint8_t *chaddr) {
    char* mac_address = malloc(17 * sizeof(char));
    sprintf(mac_address,"%02x:%02x:%02x:%02x:%02x:%02x", chaddr[0], chaddr[1], chaddr[2], chaddr[3], chaddr[4], chaddr[5]);
    int i = 0;
    for (i = 0; i < IP_POOL_SIZE; i++) {
        if (pool[i].mac_address != NULL) {
            if(strcmp(mac_address, (pool[i].mac_address)) == 0) {
                pool[i].status = TAKEN;
                *(pool[i].timestamp) = *(pool[i].timestamp) + LEASE_TIME;
                return &pool[i];
            }
        }
        
    }
    
    for (i = 0; i < IP_POOL_SIZE - 2; i++) {
        if (pool[i].status == FREE) {
            pool[i].timestamp = malloc(sizeof(time_t));
            pool[i].status = IN_PROGRESS;
            pool[i].mac_address = mac_address;
            *(pool[i].timestamp) = time(NULL);
            return &pool[i];
        }
    }
    

    return NULL;
}

int create_offer_packet(dhcp_packet *offer_packet, dhcp_packet *discovery_packet, ip_address* pool, char* serverIP) {
    memset(offer_packet, 0, sizeof(dhcp_packet));
    offer_packet->op = 2; // Boot reply
    offer_packet->htype = 1; // Ethernet
    offer_packet->hlen = 6; // MAC Address length
    offer_packet->hops = 0; // No relay agents
    offer_packet->xid = discovery_packet->xid;
    offer_packet->secs = 0; // No seconds elapsed
    offer_packet->flags = htons(0x8000); // Broadcast flag
    offer_packet->siaddr.s_addr = inet_addr(serverIP); // Server address 
    offer_packet->ciaddr.s_addr = 0;
    offer_packet->giaddr.s_addr = 0;
    memcpy(offer_packet->chaddr, discovery_packet->chaddr, 6); // Copy client MAC
    memset(&offer_packet[6], 0, 10);
    memset(offer_packet->file, 0, sizeof(offer_packet->file));
    memset(offer_packet->sname, 0, sizeof(offer_packet->sname));
    offer_packet->cookie = htonl(0x63825363);

    print_mac_address(offer_packet->chaddr);
    offer_packet->options[0] = 53; // DHCP message type option
    offer_packet->options[1] = 1; // Length of the option data
    offer_packet->options[2] = 2; // DHCP offer message type

    uint8_t subnetOctets[4];
    parseStringToOctets(SUBNET, subnetOctets);

    offer_packet->options[3] = 1; // Subnet mask option
    offer_packet->options[4] = 4; // Length of the option data
    offer_packet->options[5] = subnetOctets[0];
    offer_packet->options[6] = subnetOctets[1];
    offer_packet->options[7] = subnetOctets[2];
    offer_packet->options[8] = subnetOctets[3];
    uint8_t serverIpOctets[4];
    parseStringToOctets(serverIP, serverIpOctets);
    

    offer_packet->options[9] = 3; // Router options
    offer_packet->options[10] = 4; // Option data length
    offer_packet->options[11] = serverIpOctets[0];
    offer_packet->options[12] = serverIpOctets[1];
    offer_packet->options[13] = serverIpOctets[2];
    offer_packet->options[14] = serverIpOctets[3];

    offer_packet->options[15] = 51; // Lease Time option
    offer_packet->options[16] = 4; // Option data length
    *(uint32_t *)(&offer_packet->options[17]) = htonl(LEASE_TIME);
    offer_packet->options[21] = 58; // Renewal Time option
    offer_packet->options[22] = 4; // Option data length
    *(uint32_t *)(&offer_packet->options[23]) = htonl(RENEWAL_TIME); // Convert renewal time to network byte order and store in packet

    offer_packet->options[27] = 54;
    offer_packet->options[28] = 4;
    offer_packet->options[29] = serverIpOctets[0];
    offer_packet->options[30] = serverIpOctets[1];
    offer_packet->options[31] = serverIpOctets[2];
    offer_packet->options[32] = serverIpOctets[3];

    offer_packet->options[33] = 255; // End option


    ip_address* free_ip = get_free_ip(pool, offer_packet->chaddr);
    char* ip_str = malloc(16 * sizeof(char));
    if (free_ip != NULL) {
        sprintf(ip_str, "%d.%d.%d.%d", free_ip->octets[0], free_ip->octets[1], free_ip->octets[2], free_ip->octets[3]);
        offer_packet->yiaddr.s_addr = inet_addr(ip_str);
        
    } else {
        printf("No free addresses available!\n");
    }

    return sizeof(dhcp_packet);
}


uint32_t extract_requested_ip(dhcp_packet *packet) {
    uint8_t *p = packet->options;
    while (*p != 0xff) {
        if (*p == 50) {
            return *(uint32_t *)(p + 2);
        }
        p += p[1] + 2;
    }
    return 0;
}

int create_ack_packet(dhcp_packet *ack_packet, dhcp_packet *request_packet, uint32_t requested_ip, ip_address* pool, char* serverIP) {
    printf("Requested IP address: %s\n", inet_ntoa(*(struct in_addr *)&requested_ip));
    memset(ack_packet, 0, sizeof(dhcp_packet));
    ack_packet->op = 2; // Boot reply
    ack_packet->htype = 1; // Ethernet
    ack_packet->hlen = 6; // MAC Address length
    ack_packet->hops = 0; // No relay agents
    ack_packet->xid = request_packet->xid;
    ack_packet->secs = 0; // No seconds elapsed
    ack_packet->flags = htons(0x8000); // Broadcast flag
    ack_packet->siaddr.s_addr = inet_addr(serverIP); // Server address 
    ack_packet->ciaddr.s_addr = 0;
    ack_packet->giaddr.s_addr = 0;
    memcpy(ack_packet->chaddr, request_packet->chaddr, 6); // Copy client MAC
    memset(&ack_packet[6], 0, 10);
    memset(ack_packet->file, 0, sizeof(ack_packet->file));
    memset(ack_packet->sname, 0, sizeof(ack_packet->sname));
    ack_packet->cookie = htonl(0x63825363);

    ack_packet->options[0] = 53; // DHCP message type option
    ack_packet->options[1] = 1; // Length of the option data
    ack_packet->options[2] = 5; // DHCP ACK message type

    uint8_t subnetOctets[4];
    parseStringToOctets(SUBNET, subnetOctets);

    ack_packet->options[3] = 1; // Subnet mask option
    ack_packet->options[4] = 4; // Length of the option data
    ack_packet->options[5] = subnetOctets[0];
    ack_packet->options[6] = subnetOctets[1];
    ack_packet->options[7] = subnetOctets[2];
    ack_packet->options[8] = subnetOctets[3];

    uint8_t serverIpOctets[4];
    parseStringToOctets(serverIP, serverIpOctets);

    ack_packet->options[9] = 3; // Router options
    ack_packet->options[10] = 4; // Option data length
    ack_packet->options[11] = serverIpOctets[0];
    ack_packet->options[12] = serverIpOctets[1];
    ack_packet->options[13] = serverIpOctets[2];
    ack_packet->options[14] = serverIpOctets[3];

    ack_packet->options[15] = 51; // Lease Time option
    ack_packet->options[16] = 4; // Option data length
    *(uint32_t *)(&ack_packet->options[17]) = htonl(LEASE_TIME);
    ack_packet->options[21] = 58; // Renewal Time option
    ack_packet->options[22] = 4; // Option data length
    *(uint32_t *)(&ack_packet->options[23]) = htonl(RENEWAL_TIME); // Convert renewal time to network byte order and store in packet

    ack_packet->options[27] = 54;
    ack_packet->options[28] = 4;
    ack_packet->options[29] = serverIpOctets[0];
    ack_packet->options[30] = serverIpOctets[1];
    ack_packet->options[31] = serverIpOctets[2];
    ack_packet->options[32] = serverIpOctets[3];

    ack_packet->options[33] = 255; // End option


    ip_address* free_ip = get_free_ip(pool, ack_packet->chaddr);
    char* ip_str = malloc(16 * sizeof(char));
    if (free_ip != NULL) {
        sprintf(ip_str, "%d.%d.%d.%d", free_ip->octets[0], free_ip->octets[1], free_ip->octets[2], free_ip->octets[3]);
        ack_packet->yiaddr.s_addr = inet_addr(ip_str);   
    } else {
        printf("No free addresses available!\n");
    }

    return sizeof(dhcp_packet);
}



int main() {
    ip_address pool[IP_POOL_SIZE];
    initialize_ip_pool(pool);

    char* serverIP = getInterfaceIP(INTERFACE);
    printf("Server IP Address: %s\n", serverIP);

    int sockfd;
    struct sockaddr_in server_addr, client_addr;
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
            clean_ip_pool(pool);
            int offer_packet_size = create_offer_packet(&offer_packet, &discovery_packet, pool, serverIP);
            sendto(sockfd, &offer_packet, offer_packet_size, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            printf("Sending DHCP offer packet to client\n");
        }

        if (discovery_packet.options[2] == DHCP_REQUEST) {
            printf("DHCP Request message received from client\n");
            uint32_t requested_ip = extract_requested_ip(&discovery_packet);
            
            int ack_packet_size = create_ack_packet(&ack_packet, &discovery_packet, requested_ip, pool, serverIP);
            sendto(sockfd, &ack_packet, ack_packet_size, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            printf("Sending DHCP ack packet to client\n");
        }
    }

    return 0;
}
