CC       = gcc
CFLAGS   = -std=c11 -Wall -Wextra -pthread -g
TSAN_FLAGS = -std=c11 -Wall -Wextra -pthread -g -fsanitize=thread
ASAN_FLAGS = -std=c11 -Wall -Wextra -pthread -g -fsanitize=address,undefined
SRC      = src/main.c src/ta.c src/student.c src/util.c
OBJ      = $(SRC:.c=.o)
TARGET   = ta
TARGET_TSAN = ta_tsan
TARGET_ASAN = ta_asan

.PHONY: all clean tsan asan test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $(SRC)

tsan: $(SRC)
	$(CC) $(TSAN_FLAGS) -o $(TARGET_TSAN) $(SRC)

asan: $(SRC)
	$(CC) $(ASAN_FLAGS) -o $(TARGET_ASAN) $(SRC)

clean:
	rm -f $(TARGET) $(TARGET_TSAN) $(TARGET_ASAN) src/*.o

test: $(TARGET)
	@echo "Running quick 3-student test..."
	./$(TARGET) -s 3 -c 3 -n 2

