#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "fifo.h"

int create_fifo(struct bgw_fifo *fifo) {
	char tmpdir[] = "/tmp/bgw-XXXXXX*";	// allocate length + 1 for mkdtemp
	tmpdir[sizeof(tmpdir) - 2] = 0;

	if (mkdtemp(tmpdir) == NULL) {
		return -1;
	}

	sprintf(fifo->name, "%s/%s", tmpdir, "script");

	if (mkfifo(fifo->name, S_IWUSR | S_IRUSR) < 0) {
		return -1;
	}

	return 0;
}

int remove_fifo(struct bgw_fifo *fifo) {
	char dir[256];

	if (unlink(fifo->name)) {
		return -1;
	}

	strcpy(dir, fifo->name);

	dirname(dir);
	if (rmdir(dir)) {
		return -1;
	}

	return 0;
}

