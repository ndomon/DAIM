#include "dep.h"

char buffer[10240];
int read_buffer_len;
int write_buffer_len;
struct ofp_action_output *p_action_output;
struct ofp_flow_mod *p_flow_mod;

int main (int argc, char *argv[])
{
    int f = open ("mod.bin", O_RDONLY);
    
    if (f == -1) {
        printf ("Opening error\n");
        exit (1);
    }
    
    memset (buffer, '\0', sizeof(buffer));
    read_buffer_len = read (f, buffer, sizeof (buffer));
    
    p_flow_mod = (struct ofp_flow_mod *) buffer;
    p_action_output = (struct ofp_action_output *) p_flow_mod->actions;
    
    printf ("DL Type: %X\nN Proto: %X\nSRC %X\nDST %X\nTP SRC %u\nTP DST %u\nWild %u\nP1: %u\nP2: %u\nP2: %u\nTOS %u\n", 
            ntohs(p_flow_mod->match.dl_type), p_flow_mod->match.nw_proto,ntohl (p_flow_mod->match.nw_src), 
            ntohl (p_flow_mod->match.nw_dst), ntohs (p_flow_mod->match.tp_src), ntohs (p_flow_mod->match.tp_dst), ntohl (p_flow_mod->match.wildcards), p_flow_mod->match.pad1[0], p_flow_mod->match.pad2[0], p_flow_mod->match.pad2[1], p_flow_mod->match.nw_tos);
    printf ("Type: %X\nLen: %u\nPort: %u\nM Len: %u\n", ntohs (p_action_output->type), ntohs (p_action_output->len), ntohs (p_action_output->port), ntohs (p_action_output->max_len));
    printf ("Out port: %u\nCommand: %u\n", ntohs(p_flow_mod->out_port), ntohs(p_flow_mod->command));
    close (f);
}