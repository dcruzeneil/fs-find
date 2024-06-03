CFLAGS=-Wall -pedantic

fs-find : fs-find.c
	gcc $(CFLAGS) -o fs-find fs-find.c

.PHONY: clean
clean:
	rm -f fs-find
