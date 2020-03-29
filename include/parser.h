#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>

struct route_table_entry {
	struct in_addr prefix;
	struct in_addr next_hop;
	struct in_addr mask;
	int interface;
};

struct arp_table_entry {
	struct in_addr ip;
	uint8_t mac[6];
};

int read_rtable(struct route_table_entry *rtable);
int number_of_entries();
void parse_entry(char* entry, struct route_table_entry* table_entry);

#endif
