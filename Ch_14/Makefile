CFLAGS = -Wall -g
PROGS = null leak overflow use_after_free bad_free vector
all: $(PROGS)
%: %.c
	gcc $(CFLAGS) -o $@ $<
clean:
	rm -f $(PROGS)
