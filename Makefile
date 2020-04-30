
BINS=ssfs
DIRS=mount
all: $(BINS)

%: %.c
	gcc -Wall `pkg-config fuse --cflags` $^ -o $@ `pkg-config fuse --libs`

r:
	rm -f $(BINS)

m:
	./$(BINS) ./$(DIRS)

u:
	sudo umount -l ./$(DIRS)
