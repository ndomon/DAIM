#include "com.h"

struct ofp_header *of_header = NULL;
struct ofp_switch_config *of_switch_config = NULL;
struct ofp_packet_out *of_packet_out = NULL;
struct ofp_flow_mod *of_flow_mod = NULL;
struct ofp_action_output *of_action_output = NULL;

void create_of_hello (void *packet, uint32_t transaction_id)
{
   of_header = (struct ofp_header *) packet;
   of_header->version = OFP_VERSION;
   of_header->type = OFPT_HELLO;
   of_header->xid = transaction_id;
   of_header->length = htons (sizeof (struct ofp_header));
}

void create_of_echo_reply (void *packet, uint32_t transaction_id)
{
   of_header = (struct ofp_header *) packet;
   of_header->version = OFP_VERSION;
   of_header->type = OFPT_ECHO_REPLY;
   of_header->xid = transaction_id;
   of_header->length = htons (sizeof (struct ofp_header));
}

void create_of_features_request (void *packet, uint32_t transaction_id)
{
   of_header = (struct ofp_header *) packet;
   of_header->version = OFP_VERSION;
   of_header->type = OFPT_FEATURES_REQUEST;
   of_header->xid = transaction_id;
   of_header->length = htons (sizeof (struct ofp_header));
}

void create_of_switch_config (void *packet, uint32_t transaction_id, uint16_t flags, uint16_t miss_send_len)
{
   of_switch_config = (struct ofp_switch_config *) packet;
   of_switch_config->header.version = OFP_VERSION;
   of_switch_config->header.type = OFPT_SET_CONFIG;
   of_switch_config->header.xid = transaction_id;
   of_switch_config->header.length = htons (sizeof (ofp_switch_config));
   of_switch_config->flags = flags;
   of_switch_config->miss_send_len = miss_send_len;
}

void create_of_packet_out (void *packet, uint32_t transaction_id, uint32_t buffer_id, uint16_t input_port, uint16_t actions_length, uint16_t data_length)
{
   of_packet_out = (struct ofp_packet_out *) packet;
   of_packet_out->header.version = OFP_VERSION;
   of_packet_out->header.type = OFPT_PACKET_OUT;
   of_packet_out->header.xid = transaction_id;
   of_packet_out->header.length = htons (sizeof (struct ofp_packet_out) + actions_length + data_length);
   of_packet_out->buffer_id = buffer_id;
   of_packet_out->in_port = input_port;
   of_packet_out->actions_len = htons (actions_length);
}

void create_of_flow_mod (void *packet, uint32_t transaction_id, uint16_t input_port, uint16_t out_port, uint32_t buffer_id, uint16_t command, uint64_t cookie, uint16_t flags, uint16_t hard_timeout, uint16_t idle_timeout, uint16_t priority, uint32_t wildcards, uint32_t ip_addr_src, uint32_t ip_addr_dst, uint16_t ip_port_src, uint16_t ip_port_dst, uint8_t ip_protocol, uint8_t ip_tos, uint16_t vlan, uint8_t vlan_priority, uint8_t *eth_src, uint8_t *eth_dst, uint16_t eth_type, uint16_t acts_length)
{
   of_flow_mod = (struct ofp_flow_mod *) packet;
   of_flow_mod->header.version = OFP_VERSION;
   of_flow_mod->header.type = OFPT_FLOW_MOD;
   of_flow_mod->header.xid = transaction_id;
   of_flow_mod->header.length = htons (sizeof (struct ofp_flow_mod) + acts_length);
   of_flow_mod->match.in_port = input_port;
   of_flow_mod->match.wildcards = wildcards;
   of_flow_mod->match.nw_proto = ip_protocol;
   memcpy (of_flow_mod->match.dl_src, eth_src, sizeof(uint8_t) * OFP_ETH_ALEN);
   memcpy (of_flow_mod->match.dl_dst, eth_dst, sizeof(uint8_t) * OFP_ETH_ALEN);
   of_flow_mod->match.pad1[0] = 0;
   of_flow_mod->match.pad2[0] = 0;
   of_flow_mod->match.pad2[1] = 0;
   of_flow_mod->match.dl_type = eth_type;
   of_flow_mod->match.nw_src = ip_addr_src;
   of_flow_mod->match.nw_dst = ip_addr_dst;
   of_flow_mod->match.nw_tos = ip_tos;
   of_flow_mod->match.tp_src = ip_port_src;
   of_flow_mod->match.tp_dst = ip_port_dst;
   of_flow_mod->match.dl_vlan = vlan;
   of_flow_mod->match.dl_vlan_pcp = vlan_priority;
   of_flow_mod->out_port = out_port;
   of_flow_mod->buffer_id = buffer_id;
   of_flow_mod->command = command;
   of_flow_mod->cookie = cookie;
   of_flow_mod->flags = flags;
   of_flow_mod->hard_timeout = hard_timeout;
   of_flow_mod->idle_timeout = idle_timeout;
   of_flow_mod->priority = priority;
}

void create_of_action_output (void *packet, uint16_t max_len, uint16_t out_port)
{
    of_action_output = (struct ofp_action_output *) packet;
    of_action_output->type = htons (OFPAT_OUTPUT);
    of_action_output->len = htons (8);
    of_action_output->max_len = max_len;
    of_action_output->port = out_port;
}

