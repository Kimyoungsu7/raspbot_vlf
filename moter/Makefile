CC = g++

CFLAGS = -w -Wall -O2
SRCS = line trace.cpp  # .c -> .cpp로 변경
PROG = test

OPENCV = `pkg-config --cflags --libs opencv`
LIBS = -lwiringPi $(OPENCV)

$(PROG): $(SRCS)
	$(CC) $(CFLAGS) -o $(PROG) $(SRCS) $(LIBS)

clean:
	rm -f $(PROG)
