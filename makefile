SOURCEC=test.c

TARGETC=test

.PHONY: all c

all: c

c:
 $(CC) $(SOURCEC) -o $(TARGETC) -lpthread

clean:
 -rm -f *.o
 -rm -f *.txt
 -rm -f $(TARGETC)
