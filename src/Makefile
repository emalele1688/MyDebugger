include ../Makefile


INCLUDE_PATH = ../include/
INCLUDE = -I$(INCLUDE_PATH)


MyDebugger.o: MyDebugger.c $(INCLUDE_PATH)MyDebugger.h
	$(CC) $(WARNING) $(CFLAGS) $(OBJ_FLAG) MyDebugger.c $(INCLUDE)

opcodesdiss.o: opcodesdiss.c $(INCLUDE_PATH)opcodesdiss.h
	$(CC) $(WARNING) $(CFLAGS) $(OBJ_FLAG) opcodesdiss.c $(INCLUDE)

main.o: main.c $(INCLUDE_PATH)MyDebugger.h
	$(CC) $(WARNING) $(CFLAGS) $(OBJ_FLAG) main.c $(INCLUDE) 