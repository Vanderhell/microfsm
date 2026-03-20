CC      ?= gcc
CFLAGS  := -std=c99 -Wall -Wextra -Wpedantic -Werror -I../include
SRC     := ../src/mfsm.c
TESTS   := test_all.c

.PHONY: all clean coverage

all: test_all
	@./test_all

test_all: $(TESTS) $(SRC) ../include/mfsm.h
	$(CC) $(CFLAGS) $(SRC) $(TESTS) -o $@

coverage: $(TESTS) $(SRC) ../include/mfsm.h
	$(CC) $(CFLAGS) --coverage $(SRC) $(TESTS) -o test_coverage
	./test_coverage
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html
	@echo "Coverage report: coverage_html/index.html"

clean:
	rm -f test_all test_coverage *.gcda *.gcno coverage.info
	rm -rf coverage_html
