#include "con.h"

// ani parameters
bool verbose = true;
bool cbench = false;

// variables for sockets
int ani_sockfd = -1;
struct sockaddr_in ani_addr;
uint16_t ani_port = DEFAULT_ANI_PORT;
int sw_sockfd = -1;
struct sockaddr_in sw_addr;
socklen_t sw_addl = 0;

// hosts and ports tables
object_list hosts;
object_list ports;

// variables for buffers
char read_buffer[3072000];
char write_buffer[10240];
char *read_bufferp = NULL;
long read_buffer_len = 0;
long write_buffer_len = 0;

// OF transaction id for packets
uint32_t transaction_id = 0;
// OF Switch datapath id
uint64_t switch_id = 0;
// OF Switch no of ports
int no_of_ports = -1;
// Variables for sharing nodes
pthread_t node_send_thread = -1;
struct switch_info sw_in;

// OpenFlow packet pointers
struct ofp_header *pheader = NULL;
struct ofp_switch_config *pswitch_config = NULL;
struct ofp_switch_features *p_features_reply = NULL;
struct ofp_packet_out *p_packet_out = NULL;
struct ofp_action_output *p_action_output = NULL;
struct ofp_packet_in *p_packet_in = NULL;
struct of_ethernet *p_packet_in_eth = NULL;
struct ofp_flow_mod *p_flow_mod = NULL;
struct arp *p_packet_in_arp = NULL;
struct iphdr *p_packet_in_ip = NULL;
struct tcphdr *p_packet_in_tcp = NULL;
struct udphdr *p_packet_in_udp = NULL;

void *node_share_func (void *message);

int create_ani_socket ()
{
    int optval;
    ani_sockfd = socket (AF_INET, SOCK_STREAM, 0);

    if (ani_sockfd < 0)
    {
        cerr << "Error: can not create ANI socket: " << strerror (errno) << endl;
        return -1;
    }

    memset (&ani_addr, '\0', sizeof (ani_addr));

    ani_addr.sin_family = AF_INET;
    ani_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    ani_addr.sin_port = htons (ani_port);

    optval = 1;
    if(setsockopt(ani_sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1)
    {
        cerr << "Error: can not set socket SO_REUSEADDR option, " << strerror (errno) << endl;
    }
    
//    /* Set the option active */
//    optval = 1;
//    if (setsockopt(ani_sockfd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int)) < 0) {
//        perror("setsockopt()");
//    }
    
    if (bind (ani_sockfd, (struct sockaddr *) &ani_addr, sizeof (ani_addr)) < 0)
    {
        cerr << "Error: can not bind ANI socket, " << strerror (errno) << endl;
        return -1;
    }

    if (listen (ani_sockfd, 1) == -1)
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
    sw_sockfd = accept (ani_sockfd, (struct sockaddr *) &sw_addr, &sw_addl);
    
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
    if (ani_sockfd > 0) close (ani_sockfd);
    if (sw_sockfd > 0) close (sw_sockfd);    
}

int communicate_with_switch ()
{
    if (read_from_switch () == -1) {
        if (sw_sockfd > 0) {
            close (sw_sockfd);
            sw_sockfd = -1;
        }
        cout << "Total host entries freed: " << hosts.free_list() << endl;
        cout << "Total port entries freed: " << ports.free_list() << endl;
        cout << "Info: trying to reconnect to the switch" << endl;
        if (create_switch_socket () == -1)
        {
            cerr << "Error: can not connect to the switch" << endl << "Exiting..." << endl;
            exit (EXIT_FAILURE);
        }
        cout << "Info: connected to the switch!" << endl;
    }
    
    if (send_information () == -1) return -1;
    return 0;
}

int read_from_switch ()
{  
    memset (read_buffer, '\0', sizeof (read_buffer));
    read_buffer_len = 0;
    read_buffer_len = recv (sw_sockfd, read_buffer, sizeof (read_buffer), 0);
    
    if (read_buffer_len < 1)
    {
        cerr << "Error: reading from the switch, " << strerror (errno) << endl;
        return -1;
    }
    else if (read_buffer_len > 0)
    {
        if (verbose == true) cout << "Info: received packet from the switch" << endl;
        return 0;
    }
}

int check_openflow (char *of_buffer)
{
    pheader = (struct ofp_header *) of_buffer;
    
    if (pheader->version == OFP_VERSION)
    {
        if (verbose == true) cout << "Info: OpenFlow packet detected" << endl;
        return 0;
    }
    else
    {
        if (verbose == true) cerr << "Error: packet is not recognised" << endl;
        return -1;
    }
}

int send_information ()
{
    read_bufferp = read_buffer;
    if (cbench == true)
    {
        long openflow_packet_size = 0;
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
    }
    else {
        if (check_openflow (read_bufferp) == -1) {
            if (pheader->type == OFPT_HELLO)
            {
                if (action_hello (read_bufferp) == -1) return -1;
            }
            return 0;
        }
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
    }
    return 0;
}

int action_hello (char *of_buffer)
{
    pheader = (struct ofp_header *) of_buffer;
    transaction_id = ntohl (pheader->xid);
    memset (write_buffer, '\0', sizeof (write_buffer));
    create_of_hello (write_buffer, htonl (transaction_id));
    pheader = (struct ofp_header *) write_buffer;
    write_buffer_len = send (sw_sockfd, write_buffer, ntohs(pheader->length), 0);
    
    if (write_buffer_len == 0)
    {
        cerr << "Error: can not write hello to the switch: " << strerror (errno) << endl;
        return -1;
    }
    else if (write_buffer_len > 0)
    {
        if (verbose == true) cout << "Info: sent hello to the switch" << endl;
        transaction_id++;
        create_of_features_request (write_buffer, htonl (transaction_id));
        pheader = (struct ofp_header *) write_buffer;
        write_buffer_len = send (sw_sockfd, write_buffer, ntohs(pheader->length), 0);
        
        if (write_buffer_len == 0)
        {
            cerr << "Error: can not write feature request to the switch: " << strerror (errno) << endl;
            return -1;
        }
        else if (write_buffer_len > 0)
        {
            if (verbose == true) cout << "Info: features request sent to the switch" << endl;
            return 0;
        }
    }
}

int action_echo (char *of_buffer)
{
    pheader = (struct ofp_header *) of_buffer;
    transaction_id = ntohl (pheader->xid);
    memset (write_buffer, '\0', sizeof (write_buffer));
    create_of_echo_reply (write_buffer, htonl (transaction_id));
    pheader = (struct ofp_header *) write_buffer;
    
    write_buffer_len = send (sw_sockfd, write_buffer, ntohs(pheader->length), 0);
    
    if (write_buffer_len == 0)
    {
        cerr << "Error: can not write echo reply to the switch: " << strerror (errno) << endl;
        return -1;
    }
    else if (write_buffer_len > 0)
    {
        if (verbose == true) cout << "Info: echo reply sent to the switch" << endl;
        return 0;
    }
}

int action_features_reply (char *of_buffer)
{
    p_features_reply = (struct ofp_switch_features *) of_buffer;
    transaction_id = ntohl (p_features_reply->header.xid);
    
    no_of_ports = ((ntohs(p_features_reply->header.length)) - (sizeof (struct ofp_switch_features))) / (sizeof (struct ofp_phy_port));
    
    cout << "Info: there are " << no_of_ports << " ports on the switch" << endl;
    
    if (no_of_ports == 0) return -1;
    
    switch_id = be64toh(p_features_reply->datapath_id);
    
    sw_in.datapath_id = switch_id;
    sw_in.no_of_ports = no_of_ports;

    if (pthread_create (&node_send_thread, NULL, node_share_func, &sw_in) < 0) cerr << "Error: can not create thread for node sharing"<< endl;
    
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
    transaction_id++;
    create_of_switch_config (write_buffer, htonl (transaction_id), htons(0), htons (128));
    pswitch_config = (struct ofp_switch_config *) write_buffer;
    
    write_buffer_len = send (sw_sockfd, write_buffer, ntohs(pswitch_config->header.length), 0);
            
    if (write_buffer_len < 0)
    {
        cerr << "Error: can not write config to the switch: " << strerror (errno) << endl;
        return -1;
    }
    else if (write_buffer_len > 0)
    {
        if (verbose == true) cout << "Info: config request sent to the switch" << endl;
        return 0;
    }
}

int action_packout (char *of_buffer)
{
    uint32_t buffer_id = 0;
    uint16_t in_port = 0;
    uint8_t src[OFP_ETH_ALEN];
    uint8_t dst[OFP_ETH_ALEN];
    uint32_t ip_src = 0;
    uint32_t ip_dst = 0;
    u_int16_t tcp_port_src = 0;
    u_int16_t tcp_port_dst = 0;
    u_int16_t udp_port_src = 0;
    u_int16_t udp_port_dst = 0;
    struct switch_host new_host;
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
        if (cbench == true) {
            memset (write_buffer, '\0', sizeof (write_buffer));  
            create_of_packet_out (write_buffer, htonl (transaction_id), htonl (buffer_id), htons (in_port), sizeof (struct ofp_action_output), 0);
            p_packet_out = (struct ofp_packet_out *) write_buffer;
            create_of_action_output (p_packet_out->actions, htons (0), htons (OFPP_FLOOD));

            write_buffer_len = send(sw_sockfd, write_buffer, ntohs (p_packet_out->header.length), 0);
        }
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
                if (verbose == true) cout << "Info: host already in the table" << endl;
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
            if (verbose == true) cout << "Info: ARP broadcast" << endl;
            memset (write_buffer, '\0', sizeof (write_buffer));
            create_of_packet_out (write_buffer, htonl (transaction_id), htonl (buffer_id), htons (in_port), sizeof (struct ofp_action_output), 0);
            p_packet_out = (struct ofp_packet_out *) write_buffer;
            create_of_action_output (p_packet_out->actions, htons (0), htons (OFPP_FLOOD));
            
            write_buffer_len = send(sw_sockfd, write_buffer, ntohs (p_packet_out->header.length), 0);
        }
        else
        {
            uint16_t out_port = 0;
            uint8_t op = 0;
            hosts.list_rewind();
            hostp = (struct switch_host *) hosts.get_object();

            while (hostp != NULL) {
                if (memcmp (hostp->mac_addr, dst, sizeof (uint8_t) * MAC_ADDR_LEN) == 0) {
                    if (verbose == true) cout << "Info: host found in the table" << endl;
                    out_port = hostp->port_no;
                    break;
                }
                hostp = (struct switch_host *) hosts.get_object();
            }
            
            if (out_port != 0) {
                p_packet_in_eth = (struct of_ethernet *) p_packet_in->data;
                p_packet_in_eth++;
                p_packet_in_arp = (struct arp *) p_packet_in_eth;
                op = ntohs (p_packet_in_arp->arp_op);

                memset(write_buffer, '\0', sizeof (write_buffer));            
                create_of_flow_mod (write_buffer, htonl (transaction_id), htons(in_port), htons (0), htonl (buffer_id), htons (OFPFC_ADD), htobe64 (0), htons (0), htons (0), htons (60), htons (0), htonl(0), htonl (ip_src), htonl (ip_dst), htons (0), htons (0), htons(op), 0, htons(65535), htons (0), src, dst, htons (ARP_DATA), sizeof (struct ofp_action_output));
                p_flow_mod = (struct ofp_flow_mod *) write_buffer;
                create_of_action_output (p_flow_mod->actions, htons (0), htons (out_port));

                write_buffer_len = send (sw_sockfd, write_buffer, ntohs(p_flow_mod->header.length), 0);    
                if (verbose == true) cout << "Info: ARP flow mod" << endl;
            }
        }
    }
    else if (ntohs(p_packet_in_eth->type) == IP_DATA) {
        uint16_t out_port = 0;
        hosts.list_rewind();
        hostp = (struct switch_host *) hosts.get_object();

        while (hostp != NULL) {
            if (memcmp (hostp->mac_addr, dst, sizeof (uint8_t) * MAC_ADDR_LEN) == 0) {
                if (verbose == true) cout << "Info: host found in the table" << endl;
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
            if (out_port != 0) {
                memset(write_buffer, '\0', sizeof (write_buffer));
                create_of_flow_mod (write_buffer, htonl (transaction_id), htons(in_port), htons (0), htonl (buffer_id), htons (OFPFC_ADD), htobe64 (0), htons (0), htons (0), htons (60), htons (0), htonl(0), htonl (ip_src), htonl (ip_dst), htons (0), htons (0), IP_TYPE_ICMP, 0, htons(65535), htons (0), src, dst, htons (IP_DATA), sizeof (struct ofp_action_output));
                p_flow_mod = (struct ofp_flow_mod *) write_buffer;
                create_of_action_output (p_flow_mod->actions, htons (0), htons (out_port));

                write_buffer_len = send (sw_sockfd, write_buffer, ntohs (p_flow_mod->header.length), 0);
                if (verbose == true) cout << "Info: ICMP flow mod" << endl;
            }
        }
        else if (p_packet_in_ip->protocol == IP_TYPE_TCP)
        {
            if (out_port != 0) {
                p_packet_in_ip++;
                p_packet_in_tcp = (struct tcphdr *) p_packet_in_ip;
                tcp_port_src = ntohs (p_packet_in_tcp->source);
                tcp_port_dst = ntohs (p_packet_in_tcp->dest);

                memset(write_buffer, '\0', sizeof (write_buffer));
                create_of_flow_mod (write_buffer, htonl (transaction_id), htons(in_port), htons (0), htonl (buffer_id), htons (OFPFC_ADD), htobe64 (0), htons (0), htons (0), htons (60), htons (0), htonl(0), htonl (ip_src), htonl (ip_dst), htons (tcp_port_src), htons (tcp_port_dst), IP_TYPE_TCP, 0, htons(65535), htons (0), src, dst, htons (IP_DATA), sizeof (struct ofp_action_output));
                p_flow_mod = (struct ofp_flow_mod *) write_buffer;
                create_of_action_output (p_flow_mod->actions, htons (0), htons (out_port));

                write_buffer_len = send (sw_sockfd, write_buffer, ntohs (p_flow_mod->header.length), 0);
                if (verbose == true) cout << "Info: TCP flow mod" << endl;
            }
        }
        else if (p_packet_in_ip->protocol == IP_TYPE_UDP)
        {
            if (out_port != 0) {
                p_packet_in_ip++;
                p_packet_in_udp = (struct udphdr *) p_packet_in_ip;
                udp_port_src = ntohs (p_packet_in_udp->source);
                udp_port_dst = ntohs (p_packet_in_udp->dest);

                memset(write_buffer, '\0', sizeof (write_buffer));
                create_of_flow_mod (write_buffer, htonl (transaction_id), htons(in_port), htons (0), htonl (buffer_id), htons (OFPFC_ADD), htobe64 (0), htons (0), htons (0), htons (60), htons (0), htonl(0), htonl (ip_src), htonl (ip_dst), htons (udp_port_src), htons (udp_port_dst), IP_TYPE_UDP, 0, htons(65535), htons (0), src, dst, htons (IP_DATA), sizeof (struct ofp_action_output));
                p_flow_mod = (struct ofp_flow_mod *) write_buffer;
                create_of_action_output (p_flow_mod->actions, htons (0), htons (out_port));

                write_buffer_len = send(sw_sockfd, write_buffer, ntohs (p_flow_mod->header.length), 0);
                if (verbose == true) cout << "Info: UDP flow mod" << endl;
            }
        }
        else if (p_packet_in_ip->protocol == IP_PROTO_UNKNOWN) {
            memset (write_buffer, '\0', sizeof (write_buffer));
            create_of_packet_out (write_buffer, htonl (transaction_id), htonl (buffer_id), htons (in_port), sizeof (struct ofp_action_output), 0);
            p_packet_out = (struct ofp_packet_out *) write_buffer;
            create_of_action_output (p_packet_out->actions, htons (0), htons (OFPP_FLOOD));

            write_buffer_len = send (sw_sockfd, write_buffer, ntohs (p_packet_out->header.length), 0);
        }
    }
    else if (ntohs(p_packet_in_eth->type) == LLDP_DATA) {
        char message[1024];
        p_packet_in_eth = (struct of_ethernet *) p_packet_in->data;
        p_packet_in_eth++;
        memcpy (message, p_packet_in_eth, sizeof(message));
        cout << "Received message from controller: " << message << endl;
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

void *node_share_func (void *message)
{
    bool threadl = false;
    int ports_count = -1;
    uint64_t datap;
    
    ports_count = ((struct switch_info *)message)->no_of_ports;
    datap = ((struct switch_info *)message)->datapath_id;
    
    send_anim:
    
    sleep (5);
    
    for (int portno = 1; portno < (ports_count + 1); portno++) {
        struct ofp_packet_out *n_packet_out = NULL;
        struct ofp_action_output *n_action_output = NULL;
        struct of_ethernet *n_packet_out_eth = NULL;
        char eth_buffer[1024];
        char n_buffer[10240];
        char message[1024];
        long sw_write_len = -1;

        memset(n_buffer, '\0', sizeof (n_buffer));
        memset(eth_buffer, '\0', sizeof (eth_buffer));

        n_packet_out_eth = (struct of_ethernet *) eth_buffer;
        n_packet_out_eth->src[0] = 0;
        n_packet_out_eth->src[1] = 0;
        n_packet_out_eth->src[2] = 0;
        n_packet_out_eth->src[3] = 0;
        n_packet_out_eth->src[4] = 0;
        n_packet_out_eth->src[5] = 1;
        n_packet_out_eth->dst[0] = 1;
        n_packet_out_eth->dst[1] = 1 << 5 | 1 << 1 | 1;
        n_packet_out_eth->dst[2] = 1 << 5;
        n_packet_out_eth->dst[3] = 0;
        n_packet_out_eth->dst[4] = 0;
        n_packet_out_eth->dst[5] = 1;
        n_packet_out_eth->type = htons(LLDP_DATA);

        n_packet_out = (struct ofp_packet_out *) n_buffer;
        n_packet_out->header.version = OFP_VERSION;
        n_packet_out->header.type = OFPT_PACKET_OUT;
        n_packet_out->header.xid = htonl (0);
        n_packet_out->buffer_id = htonl (0xffffffff);
        n_packet_out->in_port = htons (OFPP_CONTROLLER);
        n_packet_out->actions_len = htons (sizeof (struct ofp_action_output));
        n_action_output = (struct ofp_action_output *) n_packet_out->actions;
        n_action_output->type = htons (OFPAT_OUTPUT);
        n_action_output->max_len = htons (0);
        n_action_output->len = htons (8);
        n_action_output->port = htons (portno);
        memcpy (n_buffer + sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output), eth_buffer, sizeof (struct of_ethernet));

        memset (message, '\0', sizeof(message));
        sprintf(message, "%s %u", "Hello, I am controller from datapath ", datap);

        memcpy (n_buffer + sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output) + sizeof (struct of_ethernet), message, sizeof(message));
        n_packet_out->header.length = htons (sizeof (struct ofp_packet_out) + sizeof (struct ofp_action_output) + sizeof (struct of_ethernet) + sizeof(message));
        sw_write_len = send(sw_sockfd, n_buffer, ntohs (n_packet_out->header.length), 0);

        if (sw_write_len < 1) cerr << "Error: can not send controller message to port " << portno << ", " << strerror(errno) << endl;
        else cout << "Info: controller message send to port " << portno << endl;
        sleep (1);
    }

    goto send_anim;

}