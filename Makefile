CC=gcc
OUT=out
ASSETS=assets
LC3_VM=$(OUT)/lc3_vm

all: build 2048

2048: build
	$(LC3_VM) $(ASSETS)/2048.obj

build:
	mkdir -p $(OUT)
	$(CC) src/main.c -o $(LC3_VM)

.PHONY:
clean:
	rm -rf $(OUT)
