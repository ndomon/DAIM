/* 
 * File:   con.h
 * Author: Pakawat Pupatwibul
 *
 * Created on 25 July 2015, 8:50 AM
 */

#ifndef CON_H
#define	CON_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <endian.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netdb.h>

#include "arp.h"
#include "eth.h"
#include "linked.h"

#include "com.h"

using namespace std;

#define IP_TYPE_ICMP 1
#define IP_TYPE_TCP 6
#define IP_TYPE_UDP 17
#define IP_PROTO_IP 0
#define IP_PROTO_UNKNOWN 255

#define MAC_ADDR_LEN 6
    
#define DEFAULT_ANI_PORT 6633

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

struct switch_info
{
    uint64_t datapath_id;
    int no_of_ports;
};

extern uint16_t ani_port;
extern object_list hosts;
extern object_list ports;

extern pthread_t node_send_thread;

extern bool verbose;
extern bool cbench;

extern int create_ani_socket ();
extern int create_switch_socket ();
extern void close_sockets ();
extern int communicate_with_switch ();
extern int read_from_switch ();
extern int check_openflow (char *of_buffer);
extern int send_information ();
extern int action_hello (char *of_buffer);
extern int action_packout (char *of_buffer);
extern int action_echo (char *of_buffer);
extern int action_features_reply (char *of_buffer);

extern void show_hosts ();
extern void show_ports ();

#endif	/* CON_H */

