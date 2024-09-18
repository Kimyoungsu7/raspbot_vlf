CC = g++
CFLAGS = -w -Wall -O2
SRCS = line trace.c -lwiringPi 
LIBS = -lwiringPi -lpthread `pkg-config --cflags --libs opencv`
PROG = test

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(PROG)
