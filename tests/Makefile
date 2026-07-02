CC ?= gcc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=

SRC := ../src/mfsm.c
TESTS := test_all.c
WARNINGS := -Wall -Wextra -Wpedantic -Werror
BASE_CFLAGS := -std=c99 $(WARNINGS)

.PHONY: all clean coverage

all: test_all
	@./test_all

test_all: $(TESTS) $(SRC) ../include/mfsm.h
	$(CC) $(CPPFLAGS) -I../include $(BASE_CFLAGS) $(CFLAGS) $(SRC) $(TESTS) -o $@ $(LDFLAGS)

coverage: $(TESTS) $(SRC) ../include/mfsm.h
	$(CC) $(CPPFLAGS) -I../include $(BASE_CFLAGS) $(CFLAGS) --coverage $(SRC) $(TESTS) -o test_coverage $(LDFLAGS)
	./test_coverage
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory coverage_html
	@echo "Coverage report: coverage_html/index.html"

clean:
	rm -f test_all test_coverage *.gcda *.gcno coverage.info
	rm -rf coverage_html
