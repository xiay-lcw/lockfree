TARGETS=log_test_pop \
	log_test_pop_all \
	log_test_pop_all_backwards

INCLUDES=$(wildcard *.h)

CFLAGS=-g -Wall
CPPFLAGS=-std=c++14 -mcx16
LDFLAGS=-lpthread

all: $(TARGETS)

%:%.cpp $(INCLUDES)
	$(CXX) $< $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) -o $@

clean:
	rm  $(TARGETS)
