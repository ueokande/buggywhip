SOURCE_FILES=$(shell find src \( -name '*.c' -o -name '*.h' \) -a \! -name main.c)
TEST_FILES=$(shell find tests -name '*.c' -o -name '*.h')

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: bgw
bgw: $(SOURCE_FILES) src/main.c
	cc -obgw src/*.c -lreadline -lutil -Wall

.PHONY: check
check: $(SOURCE_FILES) $(TEST_FILES)
	cc -ocheck src/fifo.c tests/runner.c -Isrc -lcheck -lreadline -lutil -Wall
	./check
