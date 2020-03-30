#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "parser.h"

int read_rtable(struct route_table_entry *rtable) {
	char entry[60];
	int n;
	n = number_of_entries();

    FILE *file;
    if ((file = fopen("rtable.txt", "r")) == NULL) {
        printf("Error! opening file");
    }

    for (int i = 0; i < n; i++) {
    	fgets(entry, 60, (FILE*)file);
    	parse_entry(entry, &(rtable[i]));
    }
    
    // char rez[15];
    // inet_ntop(AF_INET, &(rtable[0].prefix), rez, 15);
    // printf("%s", rez);


    fclose(file);

    return n;
}

void parse_entry(char* entry, struct route_table_entry* table_entry) {
	char prefix[15];
    char next_hop[15];
    char mask[15];
    int interface;

    sscanf(entry, "%s %s %s %d", prefix, next_hop, mask, &interface);
    // printf("\nip:%s\nn_h:%s\nmask:%s\nint:%d\n", prefix, next_hop, mask, interface);
    inet_pton(AF_INET, prefix, &(table_entry->prefix));
    inet_pton(AF_INET, next_hop, &(table_entry->next_hop));
    inet_pton(AF_INET, mask, &(table_entry->mask));

    table_entry->interface = interface;
}

int number_of_entries() {
	FILE *fp; 
    int count = 0; 
    
    char c; 
  
   
    fp = fopen("rtable.txt", "r");
  
  
    if (fp == NULL) { 
        printf("Could not open file"); 
        return 0; 
    } 
  
   
    for (c = getc(fp); c != EOF; c = getc(fp)) {
        if (c == '\n') {
            count = count + 1; 
        }
     }
  
    
    fclose(fp); 
  
    return count;
}