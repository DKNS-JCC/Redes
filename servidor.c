/*
 *          		S E R V I D O R
 *
 *	This is an example program that demonstrates the use of
 *	sockets TCP and UDP as an IPC mechanism.
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define PUERTO 13131
#define ADDRNOTFOUND 0xffffffff
#define BUFFERSIZE 1024
#define TAM_BUFFER 512
#define MAXHOST 128

extern int errno;

void serverTCP(int s, struct sockaddr_in peeraddr_in);				   /* declare TCP server routine */
void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in); /* declare UDP server routine */
void errout(char *);												   /* declare error out routine */

int FIN = 0; // Fe!n
void finalizar(){ FIN = 1; }
int main(int argc, char *argv[])
{

	int s_TCP, s_UDP; //Descriptores de socket
	int ls_TCP;		  //Listener socket TCP

	int cc; /* contains the number of bytes read */

	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */

	struct sockaddr_in myaddr_in;	  /* for local socket address */
	struct sockaddr_in clientaddr_in; /* for peer socket address */
	int addrlen;

	fd_set readmask; 
	int numfds, s_mayor;

	char buffer[BUFFERSIZE]; /* buffer for packets to be read into */

	struct sigaction vec;

	//Crear el socket TCP 
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	myaddr_in.sin_family = AF_INET;
	/*
	 * El servidor debe escuchar en la dirección de INADDR_ANY,
	 * en lugar de en su propia dirección de Internet. Esto es
	 * generalmente una buena práctica para los servidores, porque
	 * en sistemas que están conectados a más de una red a la vez
	 * podrán tener un servidor escuchando en todas las redes a la
	 * vez. Incluso cuando el host está conectado a una sola red,
	 * esto es una buena práctica, porque hace que el programa del
	 * servidor sea más portátil.
	 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	//Enlazar el socket TCP al puerto de escucha.
	if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}

	//Escuchar en el socket TCP un máximo de 5 conexiones
	if (listen(ls_TCP, 5) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}

	//Crear el socket UDP
	s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1)
	{
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}
	
	//Enlazar el socket UDP al puerto de escucha.
	if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	/*
	 * Ahora, toda la inicialización del servidor está
	 * completa, y cualquier error del usuario ya se habrá detectado.
	 * Ahora podemos bifurcar el demonio y regresar al usuario. Necesitamos
	 * hacer un setpgrp para que el demonio ya no esté asociado
	 * con la terminal de control del usuario. Esto se hace
	 * antes de la bifurcación, para que el hijo no sea
	 * un líder del grupo de procesos. De lo contrario, si el hijo
	 * abriera una terminal, se asociaría con esa terminal como su
	 * terminal de control. Siempre es mejor que el padre haga el setpgrp.
	 */
	setpgrp();

	switch (fork())
	{
	case -1: /* Unable to fork, for some reason. */
		perror(argv[0]);
		fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
		exit(1);

	case 0: /* The child process (daemon) comes here. */

		/* Close stdin and stderr so that they will not
		 * be kept open.  Stdout is assumed to have been
		 * redirected to some logging file, or /dev/null.
		 * From now on, the daemon will not report any
		 * error messages.  This daemon will loop forever,
		 * waiting for connections and forking a child
		 * server to handle each one.
		 */

		fclose(stdin);
		fclose(stderr);

		/* Set SIGCHLD to SIG_IGN, in order to prevent
		 * the accumulation of zombies as each child
		 * terminates.  This means the daemon does not
		 * have to make wait calls to clean them up.
		 */
		if (sigaction(SIGCHLD, &sa, NULL) == -1)
		{
			perror("sigaction(SIGCHLD)");
			fprintf(stderr, "%s: unable to register the SIGCHLD signal\n", argv[0]);
			exit(1);
		}

		/* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
		vec.sa_handler = (void *)finalizar;
		vec.sa_flags = 0;
		if (sigaction(SIGTERM, &vec, (struct sigaction *)0) == -1)
		{
			perror(" sigaction(SIGTERM)");
			fprintf(stderr, "%s: unable to register the SIGTERM signal\n", argv[0]);
			exit(1);
		}

		while (!FIN)
		{
			/* Meter en el conjunto de sockets los sockets UDP y TCP */
			/*Funciones FD
				FD_ZERO: Inicializa el conjunto de descriptores de fichero especificado.
				FD_SET: Añade un descriptor de fichero al conjunto especificado.
				FD_CLR: Elimina un descriptor de fichero del conjunto especificado.
				FD_ISSET: Comprueba si un descriptor de fichero está en el conjunto especificado.
			*/

			FD_ZERO(&readmask);
			FD_SET(ls_TCP, &readmask);
			FD_SET(s_UDP, &readmask);
			/*
			Seleccionar el descriptor del socket que ha cambiado. Deja una marca en
			el conjunto de sockets (readmask)
			*/
			if (ls_TCP > s_UDP)
				s_mayor = ls_TCP;
			else
				s_mayor = s_UDP;
			//select espera a que un descriptor esté listo para ser leído o escrito o a que se produzca un error en el socket.		
			if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0) 
			{
				if (errno == EINTR)
				{
					FIN = 1;
					close(ls_TCP);
					close(s_UDP);
					perror("\nFinalizando el servidor. Se�al recibida en elect\n ");
				}
			}
			else
			{

				/* Comprobamos si el socket seleccionado es el socket TCP */
				if (FD_ISSET(ls_TCP, &readmask))
				{
					/* Note that addrlen is passed as a pointer
					 * so that the accept call can return the
					 * size of the returned address.
					 */
					/* This call will block until a new
					 * connection arrives.  Then, it will
					 * return the address of the connecting
					 * peer, and a new socket descriptor, s,
					 * for that connection.
					 */

					s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen);
					if (s_TCP == -1)
						exit(1);
					switch (fork())
					{
					case -1: /* Can't fork, just exit. */
						exit(1);
					case 0:			   /* Child process comes here. */
						close(ls_TCP); /* Close the listen socket inherited from the daemon. */
						serverTCP(s_TCP, clientaddr_in);
						exit(0);
					default: /* Daemon process comes here. */
							 /* The daemon needs to remember
							  * to close the new accept socket
							  * after forking the child.  This
							  * prevents the daemon from running
							  * out of file descriptor space.  It
							  * also means that when the server
							  * closes the socket, that it will
							  * allow the socket to be destroyed
							  * since it will be the last close.
							  */
						close(s_TCP);
					}
				}
				/* Comprobamos si el socket seleccionado es el socket UDP */
				if (FD_ISSET(s_UDP, &readmask))
				{
					/* This call will block until a new
					 * request arrives.  Then, it will
					 * return the address of the client,
					 * and a buffer containing its request.
					 * BUFFERSIZE - 1 bytes are read so that
					 * room is left at the end of the buffer
					 * for a null character.
					 */
					cc = recvfrom(s_UDP, buffer, BUFFERSIZE - 1, 0,
								  (struct sockaddr *)&clientaddr_in, &addrlen);
					if (cc == -1)
					{
						perror(argv[0]);
						printf("%s: recvfrom error\n", argv[0]);
						exit(1);
					}
					/* Make sure the message received is
					 * null terminated.
					 */
					buffer[cc] = '\0';
					serverUDP(s_UDP, buffer, clientaddr_in);
				}
			}
		} /* Fin del bucle infinito de atenci�n a clientes */
		/* Cerramos los sockets UDP y TCP */
		close(ls_TCP);
		close(s_UDP);

		printf("\nFin de programa servidor!\n");

	default: /* Parent process comes here. */
		exit(0);
	}
}

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
	int reqcnt = 0;			/* keeps count of number of requests */
	char buf[TAM_BUFFER];	/* This example uses TAM_BUFFER byte messages. */
	char hostname[MAXHOST]; /* remote host's name string */

	int len, len1, status; //Variables para controlar la longitud de los mensajes
	struct hostent *hp; /* pointer to host info for remote host */
	long timevar;		/* contains time returned by time() */

	struct linger linger; /* allow a lingering, graceful close; */
						  /* used when setting SO_LINGER */

	//La informacion del host remoto esta en la estructura clientaddr_in

	/* Get the name of the remote host. */
	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
						 hostname, MAXHOST, NULL, 0, 0);
	if (status) //Si no ha podido obtener el nombre del host remoto se muestra la direccion IP
	{
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}
	time(&timevar);
	printf("Startup from %s port %u at %s",
		   hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));

	//Cierre con linger activado para asegurar que el cliente recibe todos los datos
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1)
	{
		errout(hostname);
	}

	/* Go into a loop, receiving requests from the remote
	 * client.  After the client has sent the last request,
	 * it will do a shutdown for sending, which will cause
	 * an end-of-file condition to appear on this end of the
	 * connection.  After all of the client's requests have
	 * been received, the next recv call will return zero
	 * bytes, signalling an end-of-file condition.  This is
	 * how the server will know that no more requests will
	 * follow, and the loop will be exited.
	 */
	while (len = recv(s, buf, TAM_BUFFER, 0)) //recv devuelve el numero de bytes recibidos
	{
		if (len == -1)
			errout(hostname); /* error from recv */
							  /* The reason this while loop exists is that there
							   * is a remote possibility of the above recv returning
							   * less than TAM_BUFFER bytes.  This is because a recv returns
							   * as soon as there is some data, and will not wait for
							   * all of the requested data to arrive.  Since TAM_BUFFER bytes
							   * is relatively small compared to the allowed TCP
							   * packet sizes, a partial receive is unlikely.  If
							   * this example had used 2048 bytes requests instead,
							   * a partial receive would be far more likely.
							   * This loop will keep receiving until all TAM_BUFFER bytes
							   * have been received, thus guaranteeing that the
							   * next recv at the top of the loop will start at
							   * the begining of the next request.
							   */
		//Aqui se recibe el finger del cliente
		while (len < TAM_BUFFER)
		{
			len1 = recv(s, &buf[len], TAM_BUFFER - len, 0);
			if (len1 == -1)
				errout(hostname);
			len += len1;
		}
		reqcnt++;
		
		//Buscar en carpetas del sistema el finger
		//Llenar el buffer con el mensaje que sea
		execute_system_command("finger", buf);
		//Crea un string con el finger
		char finger[TAM_BUFFER];



		//Envio de la respuesta al cliente con el finger
		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER)
			errout(hostname);
	}

	/* The loop has terminated, because there are no
	 * more requests to be serviced.  As mentioned above,
	 * this close will block until all of the sent replies
	 * have been received by the remote host.  The reason
	 * for lingering on the close is so that the server will
	 * have a better idea of when the remote has picked up
	 * all of the data.  This will allow the start and finish
	 * times printed in the log file to reflect more accurately
	 * the length of time this connection was used.
	 */
	close(s);

	/* Log a finishing message. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("Completed %s port %u, %d requests, at %s\n",
		   hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
}

void execute_system_command(const char *command, char *output) {
    FILE *fp;
    char buffer[50];

    fp = popen(command, "r");
    if (fp == NULL) {
        perror("Error ejecutando el comando");
        strcpy(output, "Error"); // En caso de error, almacenar un mensaje predeterminado
        return;
    }

    if (fgets(buffer, sizeof(buffer), fp) != NULL) {
        strcpy(output, buffer);
        output[strcspn(output, "\n")] = '\0';
    } else {
        strcpy(output, "Sin salida");
    }
    pclose(fp);
}

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
	printf("Connection with %s aborted on error\n", hostname);
	exit(1);
}

/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in)
{
	struct in_addr reqaddr; /* for requested host's address */
	struct hostent *hp;		/* pointer to host info for requested host */
	int nc, errcode;

	struct addrinfo hints, *res;

	int addrlen;

	addrlen = sizeof(struct sockaddr_in);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/* Treat the message as a string containing a hostname. */
	/* Esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta. */
	errcode = getaddrinfo(buffer, NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		reqaddr.s_addr = ADDRNOTFOUND;
	}
	else
	{
		/* Copy address of host into the return buffer. */
		reqaddr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);

	nc = sendto(s, &reqaddr, sizeof(struct in_addr),
				0, (struct sockaddr *)&clientaddr_in, addrlen);
	if (nc == -1)
	{
		perror("serverUDP");
		printf("%s: sendto error\n", "serverUDP");
		return;
	}
}
