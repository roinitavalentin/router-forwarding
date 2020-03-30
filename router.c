#include <stdint.h>
#include <net/if_arp.h>
#include <netinet/if_ether.h>
#include <string.h>

#include "skel.h"
#include "parser.h"
#include "queue.h"

struct route_table_entry *rtable;
struct arp_table_entry *arptable;
int arptable_size;
int rtable_size;
queue q;

void sort_table() {

	struct route_table_entry *aux = malloc(sizeof(struct route_table_entry));
	unsigned int mask_i = 0;
	unsigned int mask_j = 0;
	for (int i = 0; i < rtable_size - 1; i++) {
		memcpy(&mask_i, &(rtable[i].mask), 4);
		for (int j = i + 1; j < rtable_size; j++) {
			memcpy(&mask_j, &(rtable[j].mask), 4);
			if (rtable[i].mask.s_addr < rtable[j].mask.s_addr) {
				memcpy(aux, &rtable[j], sizeof(struct route_table_entry));
				memcpy(&rtable[j], &rtable[i], sizeof(struct route_table_entry));
				memcpy(&rtable[i], aux, sizeof(struct route_table_entry));
			}
		}
		
	}
	
}

void print_add (u_char* s) {
	for (int i = 0; i < 6; i++) {
		printf("%d ", s[i]);
	}
	printf("\n");
}

uint32_t ip2int(struct in_addr ip) {
	uint32_t i = 0;
	memcpy(&i, &(ip.s_addr), 4);
	return i;
}

struct route_table_entry *get_best_route(uint32_t dest_ip) {
	for (int i = 0; i < rtable_size; i++) {
		printf("SITU\n");
		print_add(&(rtable[i].prefix.s_addr));
		print_add(&(rtable[i].mask.s_addr));
		print_add(&dest_ip);
		printf("%d\n", rtable[i].interface);
		// if (ip2int(rtable[i].prefix) & ip2int(rtable[i].mask) == dest_ip & ip2int(rtable[i].mask)) {
		if ((rtable[i].prefix.s_addr & rtable[i].mask.s_addr) == (dest_ip & rtable[i].mask.s_addr)) {
			return &rtable[i];
		}
	}
	return NULL;
}

struct arp_table_entry *get_arp_entry(uint32_t ip) {

    for (int i = 0; i < arptable_size; i++) {
		if (ip == arptable[i].ip.s_addr)
			return &arptable[i];
	}

    return NULL;
}

void print_arp_table() {
	printf("\n---------------ARP TABLE----------------\n");
	for (int i = 0; i < arptable_size; i++) {
		// print_add(&arptable[i].ip.s_addr);
		print_add(arptable[i].mac);
		printf("\n");
	}
	printf("-----------------------------------------\n");
}

void testq() {
	q = queue_create();
	int a = 5;
	int b = 12;
	int ap = a;
	queue_enq(q, &ap);
	queue_enq(q, &b);
	a = 32;
	int c = *((int*)queue_deq(q));
	printf("%d\n", c);
	c = *((int*)queue_deq(q));
	printf("%d\n", c);
}

int main(int argc, char *argv[]) {
	setvbuf(stdout, NULL, _IONBF, 0);
	packet m;
	int rc;
	
	init();
	rtable = malloc(number_of_entries() * sizeof(struct route_table_entry));
	rtable_size = read_rtable(rtable);
	sort_table();
	arptable = NULL;
	arptable_size = 0;
	q = queue_create();


	while (1) {
		print_arp_table();
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		if (!queue_empty(q)) {
			queue_enq(q, &m);
			// packet n = *((packet*) (queue_deq(q)));
			m =  *((packet*) (queue_deq(q)));
			printf("QUQUUQUQUQUUEUEUEUEUQ\n");
		}

		
		/* Students will write code here */
		printf("curul\n");

		struct ether_header *eth_hdr = (struct ether_header*) m.payload;

		unsigned long my_mac = 0;
		unsigned long source = 0;

		uint8_t *mac = malloc(6);

		get_interface_mac(m.interface, mac);
		memcpy(&my_mac, mac, 4);
		memcpy(&source, &(eth_hdr->ether_shost), 4);

		if (my_mac == source) {
			printf("PACHETUL NU ESTE PENTRU MINE ??????????\n");
			continue;
		}




		if (ntohs(eth_hdr->ether_type) == ETHERTYPE_IP) {
			printf("IP REQUEST\n");

			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));

			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);

			if (best_route == NULL) {
				printf("tristete\n");
				continue;
			}

			struct arp_table_entry *best_arp = get_arp_entry(ip_hdr->daddr);

			if (best_arp == NULL) {    // make an ARP request
				packet mp = m;
				queue_enq(q, &mp);
				printf("TRIMIT ARP\n");

				packet m_arp;
				m_arp.len = 42;
				struct ether_header *eth_hdr_m = (struct ether_header*) m_arp.payload;
				struct ether_arp *ether_arp_hdr_m = (struct ether_arp*) (m_arp.payload + sizeof(struct ether_header));
				struct arphdr *arp_hdr_m = (struct arphdr*) &(ether_arp_hdr_m->ea_hdr);

				eth_hdr_m->ether_type = htons(ETHERTYPE_ARP);
				arp_hdr_m->ar_op = htons(ARPOP_REQUEST);
				arp_hdr_m->ar_hrd = htons(ARPHRD_ETHER);
				arp_hdr_m->ar_hln = 6;
				arp_hdr_m->ar_pln = 4;
				arp_hdr_m->ar_pro = htons(0x0800);



				print_add(ether_arp_hdr_m->arp_sha);
				print_add(ether_arp_hdr_m->arp_spa);
				print_add(ether_arp_hdr_m->arp_tha);
				print_add(ether_arp_hdr_m->arp_tpa);

				memcpy(ether_arp_hdr_m->arp_tpa, &(ip_hdr->daddr), 4);
				memset(ether_arp_hdr_m->arp_tha, 0, 6);
				memset(eth_hdr_m->ether_dhost, 0xff, 6);


				uint8_t *my_mac = malloc(6);
				unsigned long my_ip;

				// int i = 1;

				for (int i = 0; i < sizeof(interfaces) / sizeof(int); i++) {
					printf("ASDFGHJ\n%d\n", i);
				
					my_ip = get_interface_ip(i);
					get_interface_mac(i, my_mac);

					memcpy(ether_arp_hdr_m->arp_sha, my_mac, 6);
					memcpy(ether_arp_hdr_m->arp_spa, &my_ip, 4);				
					memcpy(eth_hdr_m->ether_shost, my_mac, 6);

					m_arp.interface = i;
					printf("TRIMIT REMORCA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
					send_packet(i, &m_arp);
				
				}
				free(my_mac);

			} else {

				printf("ACUM POT REZOLVA IPUL YEYYYYYYYYYYYYYYYYYYYYYYYYY\n");

				// print_add(&(best_arp->prefix.s_addr));
				// print_add(best_arp->mac);
				printf("%d\n", best_route->interface);


				memcpy(&eth_hdr->ether_dhost, best_arp->mac, 6);

				uint8_t *mac = malloc(6);
				get_interface_mac(best_route->interface, mac);
				memcpy(&eth_hdr->ether_shost, mac, 6);
				free(mac);
				/* TODO 8: Forward the pachet to best_route->interface */
				send_packet(best_route->interface, &m);

			}

		} else if (ntohs(eth_hdr->ether_type) == ETHERTYPE_ARP) {
			printf("ARP\n");

			struct ether_arp *ether_arp_hdr = (struct ether_arp*) (m.payload + sizeof(struct ether_header));
			struct arphdr *arp_hdr = &(ether_arp_hdr->ea_hdr);

			u_short ar_op = ntohs(arp_hdr->ar_op);
			
			if (ar_op == ARPOP_REQUEST) {	// a host asks for the MAC adress of the router
				printf("ARP REQUEST\n");

				// printf("DA\n");
				// print_add(ether_arp_hdr->arp_sha);
				// print_add(ether_arp_hdr->arp_spa);
				// print_add(ether_arp_hdr->arp_tha);
				// print_add(ether_arp_hdr->arp_tpa);
				// printf("NU\n");

				arp_hdr->ar_op = htons(ARPOP_REPLY);

				uint8_t *mac = malloc(6);
				get_interface_mac(m.interface, mac);
				
				memcpy(ether_arp_hdr->arp_tha, ether_arp_hdr->arp_sha, 6);
				memcpy(ether_arp_hdr->arp_sha, mac, 6);
				uint8_t *aux = malloc(4);
				memcpy(aux, ether_arp_hdr->arp_tpa, 4);
				memcpy(ether_arp_hdr->arp_tpa, ether_arp_hdr->arp_spa, 4);
				memcpy(ether_arp_hdr->arp_spa, aux, 4);
				free(aux);

				memcpy(eth_hdr->ether_dhost, eth_hdr->ether_shost, 6);
				memcpy(eth_hdr->ether_shost, mac, 6);

				free(mac);

				// print_add(ether_arp_hdr->arp_sha);
				// print_add(ether_arp_hdr->arp_spa);
				// print_add(ether_arp_hdr->arp_tha);
				// print_add(ether_arp_hdr->arp_tpa);

				send_packet(m.interface, &m);
			} else if (ar_op == ARPOP_REPLY) {
				printf("ARP REPLY\n");

				struct arp_table_entry ate;
				memcpy(&ate.ip.s_addr, ether_arp_hdr->arp_spa, 4);
				memcpy(&ate.mac, ether_arp_hdr->arp_sha, 6);

				// print_add(&ate.ip.s_addr);
				print_add(ate.mac);

				struct arp_table_entry *current_entry = get_arp_entry(ate.ip.s_addr);
				if (current_entry == NULL) {
					arptable = realloc(arptable, (++arptable_size) * sizeof(struct arp_table_entry));
					memcpy(&(arptable[arptable_size - 1]), &ate, sizeof(ate));
					printf("ARP table updated.\n");
				} else if (current_entry->mac != ate.mac) {
					// memcpy(&(current_entry->mac), &ate.mac, 6);
					printf("ARP table corrected.\n");
				} else {
					printf("ARP table: an entry already exists for this IP.\n");
				}

			}


		}

		// struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));

		// struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);

		// if (best_route == NULL) {
		// 	continue;
		// }

// ----------------------------


	}

	free(rtable);
	free(arptable);
}
