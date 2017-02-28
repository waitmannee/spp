#ifndef __DCCMONI__H
#define __DCCMONI__H


struct MoniCmd
{
	char packet_order[1];
	char ID[4];	
	char packet_length[4];
	char packet[999];
};

typedef struct MoniCmd MoniCMD;

struct MoniRes
{
	char packet_order[1];
	char ID[4];		
	char packet_total[1];
	char packet_length[4];
	char packet[999];
};

typedef struct MoniRes MoniRES;


#endif
