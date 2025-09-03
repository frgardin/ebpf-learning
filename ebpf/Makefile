BPF_CLANG=clang
BPF_LD=llc
TARGET=hello.bpf.o
SRC=hello.bpf.c

all:
	$(BPF_CLANG) -O2 -g -target bpf -c $(SRC) -o $(TARGET)
