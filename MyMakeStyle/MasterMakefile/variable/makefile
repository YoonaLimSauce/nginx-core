.PHONY: clean

CC = gcc
RM = rm

EXE = simple
OBJS = main.o foo.o

$(EXE): $(OBJS)
	$(CC) -o $(EXE) $(OBJS)

%.o: %.c
	$(CC) -o $@ -c $^
# main.o: main.c
# $(CC) -o $@ -c $^

# foo.o: foo.c
# $(CC) -o $@ -c $^

clean:
	$(RM) $(EXE) $(OBJS)

