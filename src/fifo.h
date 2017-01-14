#ifndef FIFO_H
#define FIFO_H

struct bgw_fifo {
	char name[256];
};

extern int create_fifo();
extern int remove_fifo();

#endif
