/* 
 * File:   com.h
 * Author: imam
 *
 * Created on 25 July 2015, 8:27 AM
 */

#ifndef COM_H
#define	COM_H

#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

#include "openflow.h"

extern void create_of_hello (void *packet, uint32_t transaction_id);
extern void create_of_echo_reply (void *packet, uint32_t transaction_id);
extern void create_of_features_request (void *packet, uint32_t transaction_id);
extern void create_of_switch_config (void *packet, uint32_t transaction_id, uint16_t flags, uint16_t miss_send_len);
extern void create_of_packet_out (void *packet, uint32_t transaction_id, uint32_t buffer_id, uint16_t input_port, uint16_t actions_length, uint16_t data_length);
extern void create_of_flow_mod (void *packet, uint32_t transaction_id, uint16_t input_port, uint16_t out_port, uint32_t buffer_id, uint16_t command, uint64_t cookie, uint16_t flags, uint16_t hard_timeout, uint16_t idle_timeout, uint16_t priority, uint32_t wildcards, uint32_t ip_addr_src, uint32_t ip_addr_dst, uint16_t ip_port_src, uint16_t ip_port_dst, uint8_t ip_protocol, uint8_t ip_tos, uint16_t vlan, uint8_t vlan_priority, uint8_t *eth_src, uint8_t *eth_dst, uint16_t eth_type, uint16_t acts_length);
extern void create_of_action_output (void *packet, uint16_t max_len, uint16_t out_port);

#endif	/* COM_H */

