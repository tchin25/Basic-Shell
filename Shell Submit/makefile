CC = g++

CFLAGS += -std=c11
CFLAGS += -w

TARGET:=myshell

$(TARGET):myshell.cpp
	@$(CC) $(CFLAGS) $^ -o $@



.PHONY: all
all:$(TARGET)
	@./$(TARGET)

debug: myshell.cpp
	@$(CC) $(CFLAGS) $^ -o $@ -D DEBUG
	@./debug

clean:
	rm myshell
