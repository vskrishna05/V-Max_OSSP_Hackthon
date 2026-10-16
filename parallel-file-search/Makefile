CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
TARGET = parallel_search

all: $(TARGET)

$(TARGET): main.c search.h
	$(CC) $(CFLAGS) main.c -o $(TARGET)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
