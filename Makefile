TARGETS=lockfree.o \
	log_test_pop

INCLUDES=$(wildcard *.h)

CFLAGS=-g -Wall -std=c11 -mcx16
CPPFLAGS=-g -Wall -std=c++14 -mcx16
LDFLAGS=-lpthread -latomic

all: $(TARGETS)

%:%.cpp lockfree.o $(INCLUDES)
	$(CXX) lockfree.o $< $(CPPFLAGS) $(LDFLAGS) -o $@

%.o:%.c $(INCLUDES)
	$(CC) $< -c $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm -f  $(TARGETS)
