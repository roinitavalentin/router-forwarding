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

struct route_table_entry *get_best_route(uint32_t dest_ip) {
	for (int i = 0; i < rtable_size; i++) {
		if ((rtable[i].prefix.s_addr & rtable[i].mask.s_addr) == (dest_ip & rtable[i].mask.s_addr)) {
			return &rtable[i];
		}
	}
	return NULL;
}

void print_add (u_char* s) {
	for (int i = 0; i < 6; i++) {
		printf("%d ", s[i]);
	}
	printf("\n");
}

struct arp_table_entry *get_arp_entry(uint32_t ip) {

    for (int i = 0; i < arptable_size; i++) {
		if (ip == arptable[i].ip.s_addr)
			return &arptable[i];
	}

    return NULL;
}

int main(int argc, char *argv[])
{
	setvbuf(stdout, NULL, _IONBF, 0);
	packet m;
	int rc;

	init();
	rtable = malloc(number_of_entries() * sizeof(struct route_table_entry));
	rtable_size = read_rtable(rtable);
	arptable = NULL;
	arptable_size = 0;
	q = queue_create();


	while (1) {
		if (queue_empty(q)) {
			rc = get_packet(&m);
			DIE(rc < 0, "get_message");
		} else {
			m =  *((packet*) (queue_deq(q)));
		}

		
		/* Students will write code here */
		printf("curul\n");

		struct ether_header *eth_hdr = (struct ether_header*) m.payload;


		if (ntohs(eth_hdr->ether_type) == ETHERTYPE_IP) {
			printf("IP REQUEST\n");

			struct iphdr *ip_hdr = (struct iphdr *)(m.payload + sizeof(struct ether_header));

			struct route_table_entry *best_route = get_best_route(ip_hdr->daddr);

			if (best_route == NULL) {
				continue;
			}

			struct arp_table_entry *best_arp = get_arp_entry(ip_hdr->daddr);

			if (best_arp == NULL) {    // make an ARP request
				// queue_enq(q, &m);
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

				for (int i = 0; i < sizeof(interfaces) / sizeof(int); i++) {
					my_ip = get_interface_ip(i);
					get_interface_mac(i, my_mac);

					memcpy(ether_arp_hdr_m->arp_sha, my_mac, 6);
					memcpy(ether_arp_hdr_m->arp_spa, &my_ip, 4);				
					memcpy(eth_hdr_m->ether_shost, my_mac, 6);

					m_arp.interface = i;
					send_packet(i, &m_arp);
				}

				free(my_mac);
				
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

				print_add(&ate.ip.s_addr);
				print_add(ate.mac);

				struct arp_table_entry *current_entry = get_arp_entry(ate.ip.s_addr);
				if (current_entry == NULL) {
					arptable = realloc(arptable, (++arptable_size) * sizeof(struct arp_table_entry));
					memcpy(&(arptable[arptable_size - 1]), &ate, sizeof(ate));
				} else if (current_entry->mac != ate.mac) {

				} else {
					printf("ARP table: an entry already exists for this IP\n");
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
}
