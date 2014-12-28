### 
## 
## Filename: Makefile
## Author: Nipun Arora
## Created: Sun Dec 28 15:34:02 2014 (-0500)
## URL: http://www.nipunarora.net 
## 
## Description: 
######################################################################

IDIR =./include
ODIR=./obj
SRC=./src

CC=gcc
CFLAGS=-I$(IDIR)


_DEPS = queue.h shm_incr.h network_comm.h pipe_input.h input.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = queue.o shm_incr.o network_comm.o pipe_input.o proxyAgg.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))

$(ODIR)/%.o: $(SRC)/%.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

proxyAgg: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
