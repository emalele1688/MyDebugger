CC = gcc
OBJ_FLAG = -c
WARNING = -Wall
CFLAGS = -O1
LIBS = -lopcodes -lreadline
BINARY_NAME = bin/mdbg
BINARY_BUILD = -o $(BINARY_NAME)
SOURCE_PATH = src/
TESTING_PATH = testing/
INCLUDE_PATH = include/


all: MDBG TESTS

MDBG: $(BINARY_NAME)

$(BINARY_NAME): $(SOURCE_PATH)MyDebugger.o $(SOURCE_PATH)opcodesdiss.o $(SOURCE_PATH)main.o
	$(CC) $(WARNING) $(CFLAGS) $(BINARY_BUILD) $(SOURCE_PATH)MyDebugger.o $(SOURCE_PATH)opcodesdiss.o $(SOURCE_PATH)main.o $(LIBS)

$(SOURCE_PATH)MyDebugger.o: $(SOURCE_PATH)MyDebugger.c $(INCLUDE_PATH)MyDebugger.h
	make -C $(SOURCE_PATH) MyDebugger.o

$(SOURCE_PATH)opcodesdiss.o: $(SOURCE_PATH)opcodesdiss.c $(INCLUDE_PATH)opcodesdiss.h
	make -C $(SOURCE_PATH) opcodesdiss.o

$(SOURCE_PATH)main.o: $(SOURCE_PATH)main.c $(INCLUDE_PATH)MyDebugger.h
	make -C $(SOURCE_PATH) main.o

TESTS:
	make -C $(TESTING_PATH)