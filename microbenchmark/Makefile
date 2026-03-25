CC=riscv64-linux-gcc

SRC=$(wildcard *.c)
BIN=$(patsubst %.c,bin/%, $(SRC))

all: $(BIN) lps-sandbox native

# Default rule: static linking
bin/%: %.c
	$(CC) -o $@ $< -static -O2

# LPS sandbox programs (static-pie)
lps-sandbox: bin/exit_imm bin/wc bin/workload bin/fc_source bin/fc_transform bin/fc_sink

bin/exit_imm: exit_imm.c
	$(CC) -static-pie -O2 -o $@ $<

bin/wc: wc.c
	$(CC) -static-pie -O2 -o $@ $<

bin/workload: workload.c
	$(CC) -static-pie -O2 -o $@ $<

bin/fc_source: fc_source.c cycle.h
	$(CC) -static-pie -O2 -I. -o $@ $<

bin/fc_transform: fc_transform.c
	$(CC) -static-pie -O2 -o $@ $<

bin/fc_sink: fc_sink.c cycle.h
	$(CC) -static-pie -O2 -I. -o $@ $<

# Native comparison programs
native: bin/native_startup bin/native_concurrent bin/native_multicore bin/native_chain \
        bin/fc_source_native bin/fc_transform_native bin/fc_sink_native

bin/native_startup: native_startup.c
	$(CC) -static -O2 -o $@ $<

bin/native_concurrent: native_concurrent.c
	$(CC) -static -O2 -o $@ $<

bin/native_multicore: native_multicore.c
	$(CC) -static -O2 -o $@ $< -lpthread

bin/native_chain: native_chain.c cycle.h
	$(CC) -static -O2 -I. -o $@ $<

bin/fc_source_native: fc_source_native.c cycle.h
	$(CC) -static -O2 -I. -o $@ $<

bin/fc_transform_native: fc_transform_native.c
	$(CC) -static -O2 -o $@ $<

bin/fc_sink_native: fc_sink_native.c cycle.h
	$(CC) -static -O2 -I. -o $@ $<

clean:
	rm bin/*

