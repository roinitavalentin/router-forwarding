#include <stdint.h>

#include "skel.h"
#include "parser.h"

struct route_table_entry *rtable;
int rtable_size;

int main(int argc, char *argv[])
{
	packet m;
	int rc;

	// init();
	rtable = malloc(number_of_entries() * sizeof(struct route_table_entry));
	rtable_size = read_rtable(rtable);

	char rez[15];
	inet_ntop(AF_INET, &(rtable[2354].prefix), rez, 15);
	printf("%s", rez);

	while (0) {
		rc = get_packet(&m);
		DIE(rc < 0, "get_message");
		/* Students will write code here */
	}

	free(rtable);
}
