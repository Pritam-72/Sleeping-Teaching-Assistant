CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -pthread -g
SRC = src/main.c src/ta.c src/student.c src/util.c
TARGET = ta

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET) src/*.o
