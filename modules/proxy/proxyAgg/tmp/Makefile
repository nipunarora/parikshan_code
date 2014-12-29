CC=gcc
CFLAGS=-I.
DEPS = queue.h shm_incr.h network_comm.h pipe_input.h input.h
OBJ = queue.o shm_incr.o network_comm.o pipe_input.o proxyAgg.o 

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

proxyAgg: $(OBJ)
	gcc -o $@ $^ $(CFLAGS)

clean:
	rm -rf proxyAgg *.o

