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
	rm -f $(TARGET) $(TARGET_TSAN) $(TARGET_ASAN) src/*.o test-one.log test-full.log test-stress.log

test: $(TARGET)
	./$(TARGET) -s 1 -c 3 -n 1 -p 50 -t 20 > test-one.log
	grep -q "TA is sleeping" test-one.log
	grep -q "awakens TA" test-one.log
	grep -q "Clean shutdown" test-one.log
	./$(TARGET) -s 5 -c 3 -n 1 -p 0 -t 250 > test-full.log
	grep -q "chairs are full" test-full.log
	./$(TARGET) -s 8 -c 2 -n 2 -p 0 -t 0 > test-stress.log
	@test $$(grep -c "Finished helping student" test-stress.log) -eq 16
	@test $$(grep -c "Finished all programming" test-stress.log) -eq 8
	grep -q "Clean shutdown" test-stress.log
	rm -f test-one.log test-full.log test-stress.log
	@echo "tests passed"

