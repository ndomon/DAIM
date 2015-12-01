#include "dep.h"

// variables for ANI
int app_sockfd = -1;
struct sockaddr_in app_addr;
int app_port = 6633;
object_list hosts, ports;
int verbose = 1;

// variables for switch
int sw_sockfd = -1;
struct sockaddr_in sw_addr;
socklen_t sw_addl;

char read_buffer[3072000];
char write_buffer[10240];
char *read_bufferp = NULL;
long read_buffer_len;
long write_buffer_len;

// transaction id for packets
uint32_t transaction_id;
// Switch datapath id
uint64_t switch_id = 0;

// Packet headers pointers
struct ofp_header *pheader;
struct ofp_switch_config *pswitch_config;
struct ofp_switch_features *p_features_reply;
struct ofp_packet_out *p_packet_out;
struct ofp_action_output *p_action_output;
struct ofp_packet_in *p_packet_in;
struct of_ethernet *p_packet_in_eth;
struct ofp_flow_mod *p_flow_mod;
struct arp *p_packet_in_arp;
struct iphdr *p_packet_in_ip;
struct tcphdr *p_packet_in_tcp;
struct udphdr *p_packet_in_udp;

struct sigaction sigIntHandler;

void show_hosts ();
void show_ports ();

int create_app_socket ();
int create_switch_socket ();
void close_sockets ();
void exit_handler (int sig);
int communicate_with_switch ();
int read_from_switch ();
int check_openflow (char *of_buffer);
int send_information ();
int action_hello (char *of_buffer);
int action_packout (char *of_buffer);
int action_echo (char *of_buffer);
int action_features_reply (char *of_buffer);

int main (int argc, char *argv[])
{
    int c;
    if (argv[1] != NULL) app_port = atoi(argv[1]);
    
    if (argv[2] != NULL) if (strcmp (argv[2], "-q") == 0) verbose = 0;
    
    cout << "Info: creating ANI socket..." << endl;
    
    if (create_app_socket () == -1)
    {
        cerr << "Error: can not create ANI socket" << endl << "Exiting..." << endl;
        close_sockets ();
        exit (1);
    }
    cout << "Info: established ANI server!" << endl;

    cout << "Info: waiting for the switch connection..." << endl;
    
    if (create_switch_socket () == -1)
    {
        cerr << "Error: can not connect to the switch" << endl << "Exiting..." << endl;
        close_sockets ();
        exit (2);
    }
    cout << "Info: connected to the switch!" << endl;
    
    memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
    sigIntHandler.sa_handler = SIG_IGN;
    sigaction (SIGINT, &sigIntHandler, NULL);
    
    memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
    sigIntHandler.sa_handler = &exit_handler;
    sigaction (SIGINT, &sigIntHandler, NULL);

    memset (&sigIntHandler, '\0', sizeof (sigIntHandler));
    sigIntHandler.sa_handler = &exit_handler;
    sigaction (SIGTERM, &sigIntHandler, NULL);
    
    hosts.set_object_size (sizeof(struct switch_host));
    ports.set_object_size (sizeof(struct switch_port));
    
    while (1)
    {
        if (communicate_with_switch () == -1)
        {
            cerr << "Error: can not communicate with the switch" << endl;
            show_hosts ();
            hosts.free_list();
            ports.free_list();
            close_sockets ();
            exit (3);
        }
    }

    return 0;
}

int create_app_socket ()
{
    int optval;
    app_sockfd = socket (AF_INET, SOCK_STREAM, 0);

    if (app_sockfd < 0)
    {
        cerr << "Error: can not create ANI socket: " << strerror (errno) << endl;
        return -1;
    }

    memset (&app_addr, '\0', sizeof (app_addr));

    app_addr.sin_family = AF_INET;
    app_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    app_addr.sin_port = htons (app_port);

    optval = 1;
    if(setsockopt(app_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
    {
        cerr << "Error: can not set socket SO_REUSEADDR option, " << strerror (errno) << endl;
    }
    
//    /* Set the option active */
//    optval = 1;
//    if (setsockopt(app_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) < 0) {
//        perror("setsockopt()");
//    }
    
    if (bind (app_sockfd, (struct sockaddr *) &app_addr, sizeof (app_addr)) < 0)
    {
        cerr << "Error: can not bind ANI socket, " << strerror (errno) << endl;
        return -1;
    }

    if (listen (app_sockfd, 1) == -1)
    {
        cerr << "Error: can not listen to switch connection, " << strerror (errno) << endl;
        return -1;
    }

    return 0;
}

int create_switch_socket ()
{
    int optval;
    sw_addl = sizeof (sw_addr);
    memset (&sw_addr, '\0', sizeof (sw_addr));
    sw_sockfd = accept (app_sockfd, (struct sockaddr *) &sw_addr, &sw_addl);
    
    if (sw_sockfd < 0)
    {
        cerr << "Error: can not accept connection from the switch, " << strerror (errno) << endl;
        return -1;
    }
    
//    /* Set the option active */
//    optval = 1;
//    if (setsockopt(sw_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) < 0) {
//        perror("setsockopt()");
//    }
    
    optval = 1;
    if (setsockopt(sw_sockfd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(int)) < 0)
    {
        cerr << "Error: can not set protocol TCP_NODELAY option, " << strerror (errno) << endl;
    }
    
    optval = 1;
    if (setsockopt(sw_sockfd, IPPROTO_TCP, TCP_QUICKACK, &optval, sizeof(int)) < 0)
    {
        cerr << "Error: can not set protocol TCP_QUICKACK option, " << strerror (errno) << endl;
    }
    
    return 0;
}

void close_sockets ()
{
    if (app_sockfd > 0) close (app_sockfd);
    if (sw_sockfd > 0) close (sw_sockfd);    
}

void exit_handler(int sig)
{
    close_sockets();
    show_hosts ();
    hosts.free_list();
    ports.free_list();
    cout << endl << "Info: caught signal, Ctrl + C: " << sig << endl;
    exit (0);
}

int communicate_with_switch ()
{
    if (read_from_switch () == -1) return -1;
    if (send_information () == -1) return -1;
    return 0;
}

int read_from_switch ()
{  
    memset (read_buffer, '\0', sizeof (read_buffer));
    read_buffer_len = 0;
    read_buffer_len = recv (sw_sockfd, read_buffer, sizeof (read_buffer), 0);
    
    if (read_buffer_len == 0)
    {
        cerr << "Error: reading from the switch, " << strerror (errno) << endl;
        return -1;
    }
    else if (read_buffer_len > 0)
    {
        if (verbose == 1) cout << "Info: received packet from the switch" << endl;
        return 0;
    }
}

int check_openflow (char *of_buffer)
{
    pheader = (struct ofp_header *) of_buffer;
    
    if (pheader->version == 1)
    {
        if (verbose == 1) cout << "Info: OpenFlow packet detected" << endl;
        return 0;
    }
    else
    {
        if (verbose == 1) cerr << "Error: packet is not recognised" << endl;
        return -1;
    }
}

int send_information ()
{
    long openflow_packet_size = 0;
    read_bufferp = read_buffer;
    for (long counter = read_buffer_len; counter > 0; counter -= openflow_packet_size) {
        if (check_openflow (read_bufferp) == -1) return 0;
        pheader = (struct ofp_header *) read_bufferp;
        openflow_packet_size = ntohs (pheader->length);
        if (pheader->type == OFPT_HELLO)
        {
            if (action_hello (read_bufferp) == -1) return -1;
        }
        else if (pheader->type == OFPT_FEATURES_REPLY)
        {
            if (action_features_reply(read_bufferp) == -1) return -1;
        }
        else if (pheader->type == OFPT_PACKET_IN)
        {
            if (action_packout(read_bufferp) == -1) return -1;
        }
        else if (pheader->type == OFPT_ECHO_REQUEST)
        {
            if (action_echo(read_bufferp) == -1) return -1;
        }
        else if (pheader->type == OFPT_PORT_STATUS)
        {
        }
        read_bufferp += openflow_packet_size;
    }
    
    return 0;
}

int action_hello (char *of_buffer)
{
    pheader = (struct ofp_header *) of_buffer;
    transaction_id = ntohl (pheader->xid);
    memset (write_buffer, '\0', sizeof (write_buffer));
    memcpy (write_buffer, of_buffer, ntohs(pheader->length));
    
    write_buffer_len = send (sw_sockfd, write_buffer, ntohs(pheader->length), 0);
    
    if (write_buffer_len == 0)
    {
        cerr << "Error: can not write hello to the switch: " << strerror (errno) << endl;
        return -1;
    }
    else if (write_buffer_len > 0)
    {
        if (verbose == 1) cout << "Info: sent hello to the switch" << endl;
        pheader = (struct ofp_header *) write_buffer;
        pheader->type = OFPT_FEATURES_REQUEST;
        transaction_id++;
        pheader->xid = htonl (transaction_id);
        write_buffer_len = send (sw_sockfd, write_buffer, ntohs(pheader->length), 0);
        
        if (write_buffer_len == 0)
        {
            cerr << "Error: can not write feature request to the switch: " << strerror (errno) << endl;
            return -1;
        }
        else if (write_buffer_len > 0)
        {
            if (verbose == 1) cout << "Info: features request sent to the switch" << endl;
            return 0;
        }
    }
}

int action_echo (char *of_buffer)
{
    pheader = (struct ofp_header *) of_buffer;
    memset (write_buffer, '\0', sizeof (write_buffer));
    memcpy (write_buffer, of_buffer, ntohs(pheader->length));
    pheader = (struct ofp_header *) write_buffer;
    pheader->type = OFPT_ECHO_REPLY;
    
    write_buffer_len = send (sw_sockfd, write_buffer, ntohs(pheader->length), 0);
    
    if (write_buffer_len == 0)
    {
        cerr << "Error: can not write echo reply to the switch: " << strerror (errno) << endl;
        return -1;
    }
    else if (write_buffer_len > 0)
    {
        if (verbose == 1) cout << "Info: echo reply sent to the switch" << endl;
        return 0;
    }
}

int action_features_reply (char *of_buffer)
{
    int no_of_ports = -1;
    p_features_reply = (struct ofp_switch_features *) of_buffer;
    transaction_id = ntohl (p_features_reply->header.xid);
    
    no_of_ports = ((ntohs(p_features_reply->header.length)) - (sizeof (struct ofp_switch_features))) / (sizeof (struct ofp_phy_port));
    
    cout << "Info: there are " << no_of_ports << " ports on the switch" << endl;
    
    if (no_of_ports == 0) return -1;
    
    switch_id = be64toh(p_features_reply->datapath_id);

    for (int port_count = 0; port_count < no_of_ports; port_count++) 
    {
        struct switch_port new_port, *p;
        memset (&new_port, '0', sizeof (struct switch_port));
        new_port.datapath_id = switch_id;
        new_port.port_no = ntohs (p_features_reply->ports[port_count].port_no);
        new_port.mac_addr[0] = p_features_reply->ports[port_count].hw_addr[0];
        new_port.mac_addr[1] = p_features_reply->ports[port_count].hw_addr[1];
        new_port.mac_addr[2] = p_features_reply->ports[port_count].hw_addr[2];
        new_port.mac_addr[3] = p_features_reply->ports[port_count].hw_addr[3];
        new_port.mac_addr[4] = p_features_reply->ports[port_count].hw_addr[4];
        new_port.mac_addr[5] = p_features_reply->ports[port_count].hw_addr[5];
        p = (struct switch_port *) ports.add_object();
        if (p == NULL) cerr << "Error: can not create port object" << endl;
        else {
            ((struct switch_port *)p)->mac_addr[0] = new_port.mac_addr[0];
            ((struct switch_port *)p)->mac_addr[1] = new_port.mac_addr[1];
            ((struct switch_port *)p)->mac_addr[2] = new_port.mac_addr[2];
            ((struct switch_port *)p)->mac_addr[3] = new_port.mac_addr[3];
            ((struct switch_port *)p)->mac_addr[4] = new_port.mac_addr[4];
            ((struct switch_port *)p)->mac_addr[5] = new_port.mac_addr[5];
            ((struct switch_port *)p)->port_no = new_port.port_no;
            ((struct switch_port *)p)->datapath_id = new_port.datapath_id;
        }
    }

    show_ports();
    
    memset (write_buffer, '\0', sizeof (write_buffer));
    pswitch_config = (struct ofp_switch_config *) write_buffer;
    pswitch_config->header.version = 1;
    pswitch_config->header.type = OFPT_SET_CONFIG;
    transaction_id++;
    pswitch_config->header.xid = htonl (transaction_id);
    pswitch_config->header.length = htons (sizeof (ofp_switch_config));
    pswitch_config->flags = htons (0);
    pswitch_config->miss_send_len = htons (128);

    write_buffer_len = send (sw_sockfd, write_buffer, sizeof (struct ofp_switch_config), 0);
            
    if (write_buffer_len < 0)
    {
        cerr << "Error: can not write config to the switch: " << strerror (errno) << endl;
        return -1;
    }
    else if (write_buffer_len > 0)
    {
        if (verbose == 1) cout << "Info: config request sent to the switch" << endl;
        return 0;
    }
}

int action_packout (char *of_buffer)
{
    uint32_t buffer_id = 0;
    uint16_t in_port = 0;
    struct switch_host new_host;
    uint8_t src[OFP_ETH_ALEN];
    uint8_t dst[OFP_ETH_ALEN];
    uint32_t ip_src = 0;
    uint32_t ip_dst = 0;
    u_int16_t tcp_port_src = 0;
    u_int16_t tcp_port_dst = 0;
    u_int16_t udp_port_src = 0;
    u_int16_t udp_port_dst = 0;
    struct switch_host *hostp = NULL;
    bool add_host = true;    
    
    p_packet_in = (struct ofp_packet_in *) of_buffer;

    buffer_id = ntohl (p_packet_in->buffer_id);
    in_port = ntohs (p_packet_in->in_port);
    transaction_id = ntohl (p_packet_in->header.xid);

    p_packet_in_eth = (struct of_ethernet *) p_packet_in->data;
    
    memset (src, '\0', sizeof (uint8_t) * OF_ETH_ADDR_LEN);
    src[0] = p_packet_in_eth->src[0];
    src[1] = p_packet_in_eth->src[1];
    src[2] = p_packet_in_eth->src[2];
    src[3] = p_packet_in_eth->src[3];
    src[4] = p_packet_in_eth->src[4];
    src[5] = p_packet_in_eth->src[5];
    
    memset (dst, '\0', sizeof (uint8_t) * OF_ETH_ADDR_LEN);
    dst[0] = p_packet_in_eth->dst[0];
    dst[1] = p_packet_in_eth->dst[1];
    dst[2] = p_packet_in_eth->dst[2];
    dst[3] = p_packet_in_eth->dst[3];
    dst[4] = p_packet_in_eth->dst[4];
    dst[5] = p_packet_in_eth->dst[5];
        
    if (ntohs(p_packet_in_eth->type) == IP6_DATA) 
    {   
        memset (write_buffer, '\0', sizeof (write_buffer));
        p_packet_out = (struct ofp_packet_out *) write_buffer;
        p_packet_out->header.version = 1;
        p_packet_out->header.type = OFPT_PACKET_OUT;
        p_packet_out->header.xid = htonl (transaction_id);
        p_packet_out->buffer_id = htonl (buffer_id);
        p_packet_out->in_port = htons (in_port);
        p_packet_out->actions_len = htons (sizeof (struct ofp_action_output));
        p_action_output = (struct ofp_action_output *) p_packet_out->actions;
        p_action_output->type = htons (OFPAT_OUTPUT);
        p_action_output->max_len = htons (0);
        p_action_output->len = htons (8);
        p_action_output->port = htons (65531);
 
        p_packet_out->header.length = htons (sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output));
    
        write_buffer_len = send(sw_sockfd, write_buffer, sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output), 0);
    }
    else if (ntohs(p_packet_in_eth->type) == ARP_DATA)
    {
        p_packet_in_eth++;
        p_packet_in_arp = (struct arp *) p_packet_in_eth;
        
        ip_src = ntohl (p_packet_in_arp->arp_ip_source);
        ip_dst = ntohl (p_packet_in_arp->arp_ip_dest);
        
        p_packet_in_eth = (struct of_ethernet *) p_packet_in->data;
        
        hosts.list_rewind();
        hostp = (struct switch_host *) hosts.get_object();
    
        while (hostp != NULL) {
            if (memcmp (hostp->mac_addr, src, sizeof (uint8_t) * MAC_ADDR_LEN) == 0) {
                if (verbose == 1) cout << "Info: host already in the table" << endl;
                add_host = false;
                break;
            }
            hostp = (struct switch_host *) hosts.get_object();
        }
    
        if (add_host == true) {
            struct switch_host *p = NULL;
            memset (&new_host, '\0', sizeof (struct switch_host));
            new_host.datapath_id = switch_id;
            new_host.port_no = in_port;
            new_host.mac_addr[0] = src[0];
            new_host.mac_addr[1] = src[1];
            new_host.mac_addr[2] = src[2];
            new_host.mac_addr[3] = src[3];
            new_host.mac_addr[4] = src[4];
            new_host.mac_addr[5] = src[5];
    
            p = (struct switch_host *) hosts.add_object ();
            if (p == NULL) cerr << "Error: can not create object in host table" << endl;
            else {
                ((struct switch_host *)p)->mac_addr[0] = new_host.mac_addr[0];
                ((struct switch_host *)p)->mac_addr[1] = new_host.mac_addr[1];
                ((struct switch_host *)p)->mac_addr[2] = new_host.mac_addr[2];
                ((struct switch_host *)p)->mac_addr[3] = new_host.mac_addr[3];
                ((struct switch_host *)p)->mac_addr[4] = new_host.mac_addr[4];
                ((struct switch_host *)p)->mac_addr[5] = new_host.mac_addr[5];
                ((struct switch_host *)p)->port_no = new_host.port_no;
                ((struct switch_host *)p)->datapath_id = new_host.datapath_id;
                ((struct switch_host *)p)->ip_addr = ip_src;
            }
            show_hosts();
        }
        
        if ((p_packet_in_eth->dst[0] == 0xff) && (p_packet_in_eth->dst[1] == 0xff) && (p_packet_in_eth->dst[2] == 0xff) && (p_packet_in_eth->dst[3] == 0xff) && (p_packet_in_eth->dst[4] == 0xff) && (p_packet_in_eth->dst[5] == 0xff)) 
        {
            if (verbose == 1) cout << "Info: ARP broadcast" << endl;
            memset(write_buffer, '\0', sizeof (write_buffer));
            p_packet_out = (struct ofp_packet_out *) write_buffer;
            p_packet_out->header.version = 1;
            p_packet_out->header.type = OFPT_PACKET_OUT;
            p_packet_out->header.xid = htonl (transaction_id);
            p_packet_out->buffer_id = htonl (buffer_id);
            p_packet_out->in_port = htons (in_port);
            p_packet_out->actions_len = htons (sizeof (struct ofp_action_output));
            p_action_output = (struct ofp_action_output *) p_packet_out->actions;
            p_action_output->type = htons (OFPAT_OUTPUT);
            p_action_output->max_len = htons (0);
            p_action_output->len = htons (8);
            p_action_output->port = htons (65531);
            p_packet_out->header.length = htons (sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output));
    
            write_buffer_len = send(sw_sockfd, write_buffer, sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output), 0);
        }
        else
        {
            uint16_t out_port = 0;
            hosts.list_rewind();
            hostp = (struct switch_host *) hosts.get_object();

            while (hostp != NULL) {
                if (memcmp (hostp->mac_addr, dst, sizeof (uint8_t) * MAC_ADDR_LEN) == 0) {
                    if (verbose == 1) cout << "Info: host found in the table" << endl;
                    out_port = hostp->port_no;
                    break;
                }
                hostp = (struct switch_host *) hosts.get_object();
            }
            
            p_packet_in_eth = (struct of_ethernet *) p_packet_in->data;
            p_packet_in_eth++;
            p_packet_in_arp = (struct arp *) p_packet_in_eth;
            
            memset(write_buffer, '\0', sizeof (write_buffer));
            p_flow_mod = (struct ofp_flow_mod *) write_buffer;
            p_flow_mod->header.version = 1;
            p_flow_mod->header.length = htons(sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output));
            p_flow_mod->header.type = OFPT_FLOW_MOD;
            p_flow_mod->header.xid = htonl (transaction_id);
            p_flow_mod->match.in_port = htons(in_port);
            p_flow_mod->match.wildcards = htonl(0);
            memcpy (p_flow_mod->match.dl_src, src, sizeof(uint8_t) * OFP_ETH_ALEN);
            memcpy (p_flow_mod->match.dl_dst, dst, sizeof(uint8_t) * OFP_ETH_ALEN);
            p_flow_mod->match.dl_vlan = htons(65535);
            p_flow_mod->match.dl_vlan_pcp = htons (0);
            uint8_t op = ntohs (p_packet_in_arp->arp_op);
            p_flow_mod->match.nw_proto = op;
            p_flow_mod->match.nw_dst = htonl (ip_dst);
            p_flow_mod->match.nw_src = htonl (ip_src);
            p_flow_mod->match.dl_type = htons (ARP_DATA);
            p_flow_mod->cookie = htobe64 (0);
            p_flow_mod->command = htons (OFPFC_ADD);
            p_flow_mod->buffer_id = htonl (buffer_id);
            p_flow_mod->idle_timeout = htons (60);
            p_flow_mod->hard_timeout = htons (0);
            p_flow_mod->priority = htons (0);
            p_flow_mod->out_port = htons (0);
            p_flow_mod->flags = htons (0);
            p_flow_mod->match.nw_tos = 0;
            p_flow_mod->match.pad1[0] = 0;
            p_flow_mod->match.pad2[0] = 0;
            p_flow_mod->match.pad2 [1] = 0;
            p_flow_mod->match.tp_dst = htons (0);
            p_flow_mod->match.tp_src = htons (0);
            p_action_output = (struct ofp_action_output *) p_flow_mod->actions;
            p_action_output->type = htons (OFPAT_OUTPUT);
            p_action_output->max_len = htons (0);
            p_action_output->len = htons (8);
            p_action_output->port = htons (out_port);
            
            write_buffer_len = send(sw_sockfd, write_buffer, sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output), 0);    
            if (verbose == 1) cout << "Info: ARP flow mod" << endl;
        }
    }
    else if (ntohs(p_packet_in_eth->type) == IP_DATA) {
            uint16_t out_port = 0;
            hosts.list_rewind();
            hostp = (struct switch_host *) hosts.get_object();

            while (hostp != NULL) {
                if (memcmp (hostp->mac_addr, dst, sizeof (uint8_t) * MAC_ADDR_LEN) == 0) {
                    if (verbose == 1) cout << "Info: host found in the table" << endl;
                    out_port = hostp->port_no;
                    break;
                }
                hostp = (struct switch_host *) hosts.get_object();
            }
            
            p_packet_in_eth = (struct of_ethernet *) p_packet_in->data;
            p_packet_in_eth++;
            p_packet_in_ip = (struct iphdr *) p_packet_in_eth;
            
            ip_src = ntohl (p_packet_in_ip->saddr);
            ip_dst = ntohl (p_packet_in_ip->daddr);
                        
            if (p_packet_in_ip->protocol == IP_TYPE_ICMP)
            {
                memset(write_buffer, '\0', sizeof (write_buffer));
                p_flow_mod = (struct ofp_flow_mod *) write_buffer;
                p_flow_mod->header.version = 1;
                p_flow_mod->header.length = htons(sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output));
                p_flow_mod->header.type = OFPT_FLOW_MOD;
                p_flow_mod->header.xid = htonl (transaction_id);
                p_flow_mod->match.in_port = htons(in_port);
                p_flow_mod->match.wildcards = htonl(0);
                memcpy (p_flow_mod->match.dl_src, src, sizeof(uint8_t) * OFP_ETH_ALEN);
                memcpy (p_flow_mod->match.dl_dst, dst, sizeof(uint8_t) * OFP_ETH_ALEN);
                p_flow_mod->match.dl_vlan = htons(65535);
                p_flow_mod->match.dl_vlan_pcp = htons (0);
                p_flow_mod->match.nw_proto = IP_TYPE_ICMP;
                p_flow_mod->match.nw_dst = htonl (ip_dst);
                p_flow_mod->match.nw_src = htonl (ip_src);
                p_flow_mod->match.dl_type = htons (IP_DATA);
                p_flow_mod->cookie = htobe64 (0);
                p_flow_mod->command = htons (OFPFC_ADD);
                p_flow_mod->buffer_id = htonl (buffer_id);
                p_flow_mod->idle_timeout = htons (60);
                p_flow_mod->hard_timeout = htons (0);
                p_flow_mod->priority = htons (0);
                p_flow_mod->out_port = htons (0);
                p_flow_mod->flags = htons (0);
                p_flow_mod->match.nw_tos = 0;
                p_flow_mod->match.pad1[0] = 0;
                p_flow_mod->match.pad2[0] = 0;
                p_flow_mod->match.pad2 [1] = 0;
                p_flow_mod->match.tp_dst = htons (0);
                p_flow_mod->match.tp_src = htons (0);
                p_action_output = (struct ofp_action_output *) p_flow_mod->actions;
                p_action_output->type = htons (OFPAT_OUTPUT);
                p_action_output->max_len = htons (0);
                p_action_output->len = htons (8);
                p_action_output->port = htons (out_port);

                write_buffer_len = send(sw_sockfd, write_buffer, sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output), 0);
                if (verbose == 1) cout << "Info: ICMP flow mod" << endl;
            }
            else if (p_packet_in_ip->protocol == IP_TYPE_TCP)
            {
                p_packet_in_ip++;
                p_packet_in_tcp = (struct tcphdr *) p_packet_in_ip;
                tcp_port_src = ntohs (p_packet_in_tcp->source);
                tcp_port_dst = ntohs (p_packet_in_tcp->dest);
                memset(write_buffer, '\0', sizeof (write_buffer));
                p_flow_mod = (struct ofp_flow_mod *) write_buffer;
                p_flow_mod->header.version = 1;
                p_flow_mod->header.length = htons(sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output));
                p_flow_mod->header.type = OFPT_FLOW_MOD;
                p_flow_mod->header.xid = htonl (transaction_id);
                p_flow_mod->match.in_port = htons(in_port);
                p_flow_mod->match.wildcards = htonl(0);
                memcpy (p_flow_mod->match.dl_src, src, sizeof(uint8_t) * OFP_ETH_ALEN);
                memcpy (p_flow_mod->match.dl_dst, dst, sizeof(uint8_t) * OFP_ETH_ALEN);
                p_flow_mod->match.dl_vlan = htons(65535);
                p_flow_mod->match.dl_vlan_pcp = htons (0);
                p_flow_mod->match.nw_proto = IP_TYPE_TCP;
                p_flow_mod->match.nw_dst = htonl (ip_dst);
                p_flow_mod->match.nw_src = htonl (ip_src);
                p_flow_mod->match.dl_type = htons (IP_DATA);
                p_flow_mod->cookie = htobe64 (0);
                p_flow_mod->command = htons (OFPFC_ADD);
                p_flow_mod->buffer_id = htonl (buffer_id);
                p_flow_mod->idle_timeout = htons (60);
                p_flow_mod->hard_timeout = htons (0);
                p_flow_mod->priority = htons (0);
                p_flow_mod->out_port = htons (0);
                p_flow_mod->flags = htons (0);
                p_flow_mod->match.nw_tos = 0;
                p_flow_mod->match.pad1[0] = 0;
                p_flow_mod->match.pad2[0] = 0;
                p_flow_mod->match.pad2 [1] = 0;
                p_flow_mod->match.tp_dst = htons (tcp_port_dst);
                p_flow_mod->match.tp_src = htons (tcp_port_src);
                p_action_output = (struct ofp_action_output *) p_flow_mod->actions;
                p_action_output->type = htons (OFPAT_OUTPUT);
                p_action_output->max_len = htons (0);
                p_action_output->len = htons (8);
                p_action_output->port = htons (out_port);

                write_buffer_len = send(sw_sockfd, write_buffer, sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output), 0);
                if (verbose == 1) cout << "Info: TCP flow mod" << endl;
            }
            else if (p_packet_in_ip->protocol == IP_TYPE_UDP)
            {
                p_packet_in_ip++;
                p_packet_in_udp = (struct udphdr *) p_packet_in_ip;
                udp_port_src = ntohs (p_packet_in_udp->source);
                udp_port_dst = ntohs (p_packet_in_udp->dest);
                memset(write_buffer, '\0', sizeof (write_buffer));
                p_flow_mod = (struct ofp_flow_mod *) write_buffer;
                p_flow_mod->header.version = 1;
                p_flow_mod->header.length = htons(sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output));
                p_flow_mod->header.type = OFPT_FLOW_MOD;
                p_flow_mod->header.xid = htonl (transaction_id);
                p_flow_mod->match.in_port = htons(in_port);
                p_flow_mod->match.wildcards = htonl(0);
                memcpy (p_flow_mod->match.dl_src, src, sizeof(uint8_t) * OFP_ETH_ALEN);
                memcpy (p_flow_mod->match.dl_dst, dst, sizeof(uint8_t) * OFP_ETH_ALEN);
                p_flow_mod->match.dl_vlan = htons(65535);
                p_flow_mod->match.dl_vlan_pcp = htons (0);
                p_flow_mod->match.nw_proto = IP_TYPE_UDP;
                p_flow_mod->match.nw_dst = htonl (ip_dst);
                p_flow_mod->match.nw_src = htonl (ip_src);
                p_flow_mod->match.dl_type = htons (IP_DATA);
                p_flow_mod->cookie = htobe64 (0);
                p_flow_mod->command = htons (OFPFC_ADD);
                p_flow_mod->buffer_id = htonl (buffer_id);
                p_flow_mod->idle_timeout = htons (60);
                p_flow_mod->hard_timeout = htons (0);
                p_flow_mod->priority = htons (0);
                p_flow_mod->out_port = htons (0);
                p_flow_mod->flags = htons (0);
                p_flow_mod->match.nw_tos = 0;
                p_flow_mod->match.pad1[0] = 0;
                p_flow_mod->match.pad2[0] = 0;
                p_flow_mod->match.pad2 [1] = 0;
                p_flow_mod->match.tp_dst = htons (tcp_port_dst);
                p_flow_mod->match.tp_src = htons (tcp_port_src);
                p_action_output = (struct ofp_action_output *) p_flow_mod->actions;
                p_action_output->type = htons (OFPAT_OUTPUT);
                p_action_output->max_len = htons (0);
                p_action_output->len = htons (8);
                p_action_output->port = htons (out_port);

                write_buffer_len = send(sw_sockfd, write_buffer, sizeof (struct ofp_flow_mod) + sizeof (struct ofp_action_output), 0);
                if (verbose == 1) cout << "Info: UDP flow mod" << endl;
            }
            else if (p_packet_in_ip->protocol == IP_PROTO_UNKNOWN) {
                memset(write_buffer, '\0', sizeof (write_buffer));
                p_packet_out = (struct ofp_packet_out *) write_buffer;
                p_packet_out->header.version = 1;
                p_packet_out->header.type = OFPT_PACKET_OUT;
                p_packet_out->header.xid = htonl (transaction_id);
                p_packet_out->buffer_id = htonl (buffer_id);
                p_packet_out->in_port = htons (in_port);
                p_packet_out->actions_len = htons (sizeof (struct ofp_action_output));
                p_action_output = (struct ofp_action_output *) p_packet_out->actions;
                p_action_output->type = htons (OFPAT_OUTPUT);
                p_action_output->max_len = htons (0);
                p_action_output->len = htons (8);
                p_action_output->port = htons (65531);
                p_packet_out->header.length = htons (sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output));

                write_buffer_len = send(sw_sockfd, write_buffer, sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output), 0);
            }
    }
    if (write_buffer_len < 0)
    {
        cerr << "Error: can not write to the switch: " << strerror (errno) << endl;
        return -1;
    }
//    else if (write_buffer_len > 0)
//    {
//        fprintf (stdout, "Info: packet written to the switch\n");
//        return 0;
//    }
}

void show_hosts ()
{
    struct switch_host *p = NULL;
    p = (struct switch_host *) hosts.get_object();
    while (p != NULL) {
        cout << "Host MAC: " << uppercase << hex << (int) ((struct switch_host *)p)->mac_addr[0] << ":" << (int) ((struct switch_host *)p)->mac_addr[1] << ":" << (int) ((struct switch_host *)p)->mac_addr[2] << ":" << (int) ((struct switch_host *)p)->mac_addr[3] << ":" << (int) ((struct switch_host *)p)->mac_addr[4] << ":" << (int) ((struct switch_host *)p)->mac_addr[5] << dec << " Ingress Port: " << (int) ((struct switch_host *)p)->port_no << " Datapath: " << (int) ((struct switch_host *)p)->datapath_id << endl;
        p = (struct switch_host *) hosts.get_object();
    }
    hosts.list_rewind();
}

void show_ports ()
{
    struct switch_port *p = NULL;
    p = (struct switch_port *) ports.get_object();
    while (p != NULL) {
        cout << "Switch port MAC: " << uppercase << hex << (int) ((struct switch_port *)p)->mac_addr[0] << ":" << (int) ((struct switch_port *)p)->mac_addr[1] << ":" << (int) ((struct switch_port *)p)->mac_addr[2] << ":" << (int) ((struct switch_port *)p)->mac_addr[3] << ":" << (int) ((struct switch_port *)p)->mac_addr[4] << ":" << (int) ((struct switch_port *)p)->mac_addr[5] << dec << " Ingress Port: " << (int) ((struct switch_port *)p)->port_no << " Datapath: " << (int) ((struct switch_port *)p)->datapath_id << endl;
        p = (struct switch_port *) ports.get_object();
    }
    ports.list_rewind();
}