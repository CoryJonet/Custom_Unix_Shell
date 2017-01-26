CXX =	gcc
CXXFLAGS =	-O -Wall -g

all: whoosh

wl: whoosh.c
	$(CXX) $(CXXFLAGS) whoosh.c -o $@

clean:
	rm -f core *.0 whoosh
