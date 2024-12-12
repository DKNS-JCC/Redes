CC = gcc
CFLAGS = -w # Suprimir warnings
#Descomentar la siguiente linea para olivo
#LIBS = -lsocket -lnsl
#Descomentar la siguiente linea para linux
LIBS =

PROGS = servidor cliente

all: ${PROGS}

servidor: servidor.o
	${CC} ${CFLAGS} -o $@ servidor.o ${LIBS}

cliente: cliente.o
	${CC} ${CFLAGS} -o $@ cliente.o ${LIBS}

clean:
	rm -f *.o ${PROGS}
