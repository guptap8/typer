OBJS = typer.cpp

CC = g++ -std=c++17

COMPILER_FLAGS = -w -pthread

DEBUG_FLAGS = -g -DDBG

LINKER_FLAGS =

OBJ_NAME = ./bin/typer

all : $(OBJS)
	$(CC) $(OBJS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

debug : $(OBJS)
	$(CC) $(OBJS) $(DEBUG_FLAGS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)

run :
	./bin/typer
