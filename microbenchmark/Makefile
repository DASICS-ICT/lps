CC=riscv64-linux-musl-gcc

SRC=$(wildcard *.c)
BIN=$(patsubst %.c,bin/%, $(SRC))

all: $(BIN)

bin/%: %.c
	$(CC) -o $@ $< -static

clean:
	rm bin/*