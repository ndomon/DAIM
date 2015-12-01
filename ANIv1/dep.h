#ifndef APP_DEP_H
#define APP_DEP_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <unistd.h>
#include <signal.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <netdb.h>

#include <endian.h>

#include "openflow.h"
#include "openflow-ext.h"
#include "openflow-netlink.h"
#include "arp.h"
#include "eth.h"
#include "linked.h"

using namespace std;

#define IP_TYPE_ICMP 1
#define IP_TYPE_TCP 6
#define IP_TYPE_UDP 17
#define IP_PROTO_IP 0
#define IP_PROTO_UNKNOWN 255

#define MAC_ADDR_LEN 6

struct switch_port
{
    uint64_t datapath_id;
    uint16_t port_no;
    uint8_t mac_addr[MAC_ADDR_LEN];
};

struct switch_host
{
    uint64_t datapath_id;
    uint16_t port_no;
    uint8_t mac_addr[MAC_ADDR_LEN];
    uint32_t ip_addr;
};

#endif /* dep.h */