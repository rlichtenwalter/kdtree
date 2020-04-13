CC := g++
COMMON_FLAGS := -std=c++14 -Wall -Wextra -Werror -Wpedantic -Wno-unused-local-typedefs

DEBUG_FLAGS := -Og -g -fsanitize=address -fno-omit-frame-pointer
RELEASE_FLAGS := -O3 -flto -fomit-frame-pointer -D NDEBUG

ifeq ($(DEBUG),1)
	CFLAGS := $(COMMON_FLAGS) $(DEBUG_FLAGS)
else
	CFLAGS := $(COMMON_FLAGS) $(RELEASE_FLAGS)
endif

all: bin/kdtree_test bin/point_test bin/convex_polygon_test 

bin/kdtree: src/kdtree.o | bin/
	$(CC) $(CFLAGS) -o $@ $^

bin/kdtree_test: test/kdtree.o | bin/
	$(CC) $(CFLAGS) -o $@ $^

bin/point_test: test/point.o | bin/
	$(CC) $(CFLAGS) -o $@ $^

bin/convex_polygon_test: test/convex_polygon.o | bin/
	$(CC) $(CFLAGS) -o $@ $^

bin/point_in_polygon_test: test/point_in_polygon.o | bin/
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

bin/:
	mkdir -p $@

.PHONY: clean

clean:
	rm -f test/*.o
