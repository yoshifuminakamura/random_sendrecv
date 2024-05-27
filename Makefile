CC = mpifccpx
TARGET = random_sendrecv
SRC = random_sendrecv.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $@ $^

clean:
	rm -f $(TARGET)

.PHONY: all clean
