#ifndef OF_ETH_H
#define OF_ETH_H

#define OF_ETH_ADDR_LEN 6

#define ARP_DATA 0x0806
#define IP_DATA 0x0800
#define IP6_DATA 0x86dd
#define VLAN_DATA 0x8100
#define LLDP_DATA 0x88cc

// An Openflow Packet In ethernet II packet structure.
struct of_ethernet
{
    //! Destination MAC address.
    uint8_t  dst[OF_ETH_ADDR_LEN];
    //! Source MAC address.
    uint8_t  src[OF_ETH_ADDR_LEN];
    //! The packet type.
    uint16_t type;
};

#endif /* eth.h */