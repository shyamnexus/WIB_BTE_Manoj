# Makefile for Custom CAN Protocol
# Version 1.0

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
LDFLAGS = 
TARGET = custom_can_protocol
TEST_TARGET = test_can_protocol

# Source files
SOURCES = custom_can_protocol.c
HEADERS = custom_can_protocol.h
EXAMPLE_SOURCES = example_usage.c
TEST_SOURCES = test_framework.c

# Object files
OBJECTS = $(SOURCES:.c=.o)
EXAMPLE_OBJECTS = $(EXAMPLE_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

# Default target
all: $(TARGET) $(TEST_TARGET)

# Main library target
$(TARGET): $(OBJECTS)
	ar rcs lib$(TARGET).a $(OBJECTS)
	@echo "Library $(TARGET) created successfully"

# Test target
$(TEST_TARGET): $(TEST_OBJECTS) $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TEST_TARGET) $(TEST_OBJECTS) $(OBJECTS) $(LDFLAGS)
	@echo "Test executable $(TEST_TARGET) created successfully"

# Example target
example: $(EXAMPLE_OBJECTS) $(OBJECTS)
	$(CC) $(CFLAGS) -o example_usage $(EXAMPLE_OBJECTS) $(OBJECTS) $(LDFLAGS)
	@echo "Example executable created successfully"

# Compile source files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

# Run example
run-example: example
	./example_usage

# Clean build artifacts
clean:
	rm -f *.o *.a $(TARGET) $(TEST_TARGET) example_usage
	@echo "Clean completed"

# Install library (optional)
install: $(TARGET)
	@echo "Installing library to /usr/local/lib"
	sudo cp lib$(TARGET).a /usr/local/lib/
	sudo cp $(HEADERS) /usr/local/include/
	@echo "Installation completed"

# Uninstall library
uninstall:
	@echo "Uninstalling library"
	sudo rm -f /usr/local/lib/lib$(TARGET).a
	sudo rm -f /usr/local/include/$(HEADERS)
	@echo "Uninstallation completed"

# Generate documentation (requires doxygen)
docs:
	@if command -v doxygen >/dev/null 2>&1; then \
		doxygen Doxyfile; \
		echo "Documentation generated in docs/"; \
	else \
		echo "Doxygen not found. Please install doxygen to generate documentation."; \
	fi

# Code formatting (requires clang-format)
format:
	@if command -v clang-format >/dev/null 2>&1; then \
		clang-format -i *.c *.h; \
		echo "Code formatted"; \
	else \
		echo "clang-format not found. Please install clang-format to format code."; \
	fi

# Static analysis (requires cppcheck)
analyze:
	@if command -v cppcheck >/dev/null 2>&1; then \
		cppcheck --enable=all --std=c99 *.c; \
	else \
		echo "cppcheck not found. Please install cppcheck for static analysis."; \
	fi

# Memory check (requires valgrind)
memcheck: $(TEST_TARGET)
	@if command -v valgrind >/dev/null 2>&1; then \
		valgrind --leak-check=full --show-leak-kinds=all ./$(TEST_TARGET); \
	else \
		echo "valgrind not found. Please install valgrind for memory checking."; \
	fi

# Coverage report (requires gcov)
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov
coverage: clean $(TEST_TARGET)
	./$(TEST_TARGET)
	@if command -v gcov >/dev/null 2>&1; then \
		gcov *.c; \
		@if command -v lcov >/dev/null 2>&1; then \
			lcov --capture --directory . --output-file coverage.info; \
			genhtml coverage.info --output-directory coverage; \
			echo "Coverage report generated in coverage/"; \
		fi \
	else \
		echo "gcov not found. Please install gcov for coverage analysis."; \
	fi

# Help target
help:
	@echo "Available targets:"
	@echo "  all          - Build library and test executable"
	@echo "  $(TARGET)     - Build library only"
	@echo "  $(TEST_TARGET) - Build test executable"
	@echo "  example      - Build example executable"
	@echo "  test         - Run tests"
	@echo "  run-example  - Run example"
	@echo "  clean        - Remove build artifacts"
	@echo "  install      - Install library to system"
	@echo "  uninstall    - Remove library from system"
	@echo "  docs         - Generate documentation (requires doxygen)"
	@echo "  format       - Format code (requires clang-format)"
	@echo "  analyze      - Run static analysis (requires cppcheck)"
	@echo "  memcheck     - Run memory check (requires valgrind)"
	@echo "  coverage     - Generate coverage report (requires gcov)"
	@echo "  help         - Show this help message"

# Phony targets
.PHONY: all test run-example clean install uninstall docs format analyze memcheck coverage help