# compiler
CC = g++ 

MODULES = ./modules
INCLUDE = ./includes

# Compile options. Το -I<dir> λέει στον compiler να αναζητήσει εκεί include files
CPPFLAGS = -std=c++11 -Wall  -g3 -I $(INCLUDE)

LDFLAGS = -lm 
# Αρχεία .o
OBJS =   sniffer.o $(MODULES)/findUrls.o  $(MODULES)/workerFunc.o $(MODULES)/listener.o

EXEC = sniffer

$(EXEC): $(OBJS)
	$(CC) $(OBJS) -o $(EXEC) $(LDFLAGS)


run: $(EXEC)
	./$(EXEC)


clean:
	rm -f $(OBJS) $(EXEC)

