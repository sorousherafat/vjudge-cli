CC ?= gcc
CFLAGS ?= -Wall -Wextra -g
LDFLAGS ?=

SRCS = vjudge.c
OBJS = $(SRCS:.c=.o)

vjudge: $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) vjudge

