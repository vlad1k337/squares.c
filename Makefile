CC = cc
CFLAGS = -Wall -Wextra 
CPPFLAGS = -Iraylib/
LDFLAGS  = -Llib/ -l:libraylib.a -lm -fopenmp

SRC = main.c

TARGET = a.out

all: $(TARGET)

debug: CFLAGS += -g3 -ggdb 
debug: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm $(TARGET)
