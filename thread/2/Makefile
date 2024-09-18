CC = g++
CFLAGS = -w -Wall -O2
SRCS = Pthread_test1.c
LIBS = -lwiringPi -lpthread `pkg-config --cflags --libs opencv`
PROG = test

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

.PHONY: clean
clean:
	rm -f $(PROG)
