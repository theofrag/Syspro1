# compiler
CC = g++ 

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -std=c++11 -Wall  -g3
LDFLAGS = -lm

# Αρχεία .o
OBJS =   sniffer.o findUrls.o

EXEC = sniffer

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)


run: $(EXEC)
	./$(EXEC)


clean:
	rm -f $(OBJS) $(EXEC)

