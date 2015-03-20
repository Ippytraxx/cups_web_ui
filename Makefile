PROG = cups_web_ui
CFLAGS = -W -Wall -std=gnu11 -pthread -lmongoose -lcups
SOURCES = $(PROG).c

all: $(PROG)

run: $(PROG)
	./$(PROG)

$(PROG) : $(SOURCES) Makefile
	$(CC) -o $(PROG) $(SOURCES) $(CFLAGS)

clean: rm -rf $(PROG) *.o
