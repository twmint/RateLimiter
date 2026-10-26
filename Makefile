CC      := cc
CFLAGS  := -std=c11 -Wall -Wextra -Wconversion -Wpedantic -g -Isrc \
           -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer
LDFLAGS := -fsanitize=address,undefined

BIN := build/test_bucket

.PHONY: test clean

test: $(BIN)
	./$(BIN)

$(BIN): src/bucket.c tests/test_bucket.c src/bucket.h src/helper.h | build
	$(CC) $(CFLAGS) src/bucket.c tests/test_bucket.c -o $@ $(LDFLAGS)

build:
	mkdir -p build

clean:
	rm -rf build
