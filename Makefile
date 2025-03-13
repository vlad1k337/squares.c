CC = cc
CFLAGS = -Wall -Wextra 
CPPFLAGS = -Iraylib/
LDFLAGS  = -Llib/ -l:libraylib.a -lm -fopenmp

SRC = metaballs.c

TARGET = metaballs

all: $(TARGET)

debug: CFLAGS += -g3 -ggdb 
debug: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ -o $@ $(LDFLAGS)

.PHONY: clean
clean:
	rm $(TARGET)
