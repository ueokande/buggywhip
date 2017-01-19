#ifndef BITSET_H
#define BITSET_H

#include <stdlib.h>

struct bitset {
	int len;
	char *data;
};

struct bitset *bitset_new(int len) {
	struct bitset *bs = malloc(sizeof(struct bitset));
	bs->len = len;
	bs->data = calloc(len / 8 + (len % 8), sizeof(char));
	return bs;
}

void bitset_delete(struct bitset *bs) {
	free(bs->data);
	free(bs);
}

void bitset_set(struct bitset *bs, int index) {
	char *d = &bs->data[index / 8];
	*d = *d | (1 << (index % 8));
}

void bitset_reset(struct bitset *bs, int index) {
	char *d = &bs->data[index / 8];
	*d = *d & ~(1 << (index % 8));
}

char bitset_test(const struct bitset *bs, int index) {
	const char *d = &bs->data[index / 8];
	return *d & (1 << (index % 8));
}

#endif
