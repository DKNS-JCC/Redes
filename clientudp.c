/*
 *			C L I E N T U D P
 *
 *	This is an example program that demonstrates the use of
 *	sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 *	This program will request the internet address of a target
 *	host by name from the serving host.  The serving host
 *	will return the requested internet address as a response,
 *	and will return an address of all ones if it does not recognize
 *	the host name.
 *
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern int errno;

#define ADDRNOTFOUND 0xffffffff /* value returned for unknown host */
#define RETRIES 5				/* number of times to retry before givin up */
#define BUFFERSIZE 1024			/* maximum size of packets to be received */
#define PUERTO 17278
#define TIMEOUT 6
#define MAXHOST 512
/*
 *			H A N D L E R
 *
 *	This routine is the signal handler for the alarm signal.
 */
void handler()
{
	printf("Alarma recibida \n");
}

/*
 *			M A I N
 *
 *	This routine is the client which requests service from the remote
 *	"example server".  It will send a message to the remote nameserver
 *	requesting the internet address corresponding to a given hostname.
 *	The server will look up the name, and return its internet address.
 *	The returned address will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as the first parameter to the command.  The second parameter should
 *	be the the name of the target host for which the internet address
 *	is sought.
 */
int main(int argc, char *argv[])
{
	int i, errcode;
	int retry = RETRIES;			/* holds the retry count */
	int s;							/* socket descriptor */
	long timevar;					/* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in; /* for server socket address */
	struct in_addr reqaddr;			/* for returned internet address */
	int addrlen, n_retry;
	struct sigaction vec;
	char hostname[MAXHOST];
	struct addrinfo hints, *res;

	if (argc != 3)
	{
		
		exit(1);
	}

	/* Create the socket. */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Bind socket to some local address so that the
	 * server can send the reply back.  A port number
	 * of zero will be used so that the system will
	 * assign any available port number.  An address
	 * of INADDR_ANY will be used so we do not have to
	 * look up the internet address of the local host.
	 */

	// Ajustar mi direccion para el bind
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;

	// Vincular el socket
	/*Uso de bind
	 * bind(s, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in))
	 * s: socket descriptor
	 * (const struct sockaddr *)&myaddr_in: direccion del socket
	 * sizeof(struct sockaddr_in): tamaño de la direccion
	 */
	if (bind(s, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind socket\n", argv[0]);
		exit(1);
	}
	addrlen = sizeof(struct sockaddr_in);

	// Obtener la direccion del socket local
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}
	time(&timevar);
	printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

	servaddr_in.sin_family = AF_INET;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	errcode = getaddrinfo(argv[1], NULL, &hints, &res);
	if (errcode != 0)
	{
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n", argv[0], argv[1]);
		exit(1);
	}
	else
	{
		servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	// Liberar memoria de res que ya no se necesita
	freeaddrinfo(res);
	servaddr_in.sin_port = htons(PUERTO);

	vec.sa_handler = (void *)handler;
	vec.sa_flags = 0;
	if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1)
	{
		perror(" sigaction(SIGALRM)");
		fprintf(stderr, "%s: unable to register the SIGALRM signal\n", argv[0]);
		exit(1);
	}

	n_retry = RETRIES;

	while (n_retry > 0)
	{
		// Enviar el mensaje a la direccion del servidor
		/*
		Uso de sendto
		 * sendto(s, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in))
		 * s: socket descriptor
		 * argv[2]: mensaje a enviar
		 * strlen(argv[2]): tamaño del mensaje
		 * 0: flags
		 * (struct sockaddr *)&servaddr_in: direccion del servidor
		 * sizeof(struct sockaddr_in): tamaño de la direccion
		*/
		if (sendto(s, argv[2], strlen(argv[2]), 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
		{
			perror(argv[0]);
			fprintf(stderr, "%s: unable to send request\n", argv[0]);
			exit(1);
		}
		//Iniciar la alarma hasta que se reciba una respuesta
		alarm(TIMEOUT);
		
		//Si da error al recibir, se intenta de nuevo
		if (recvfrom(s, &reqaddr, sizeof(struct in_addr), 0,(struct sockaddr *)&servaddr_in, &addrlen) == -1)
		{
			if (errno == EINTR)
			{
				printf("attempt %d (retries %d).\n", n_retry, RETRIES);
				n_retry--;
			}
			else
			{
				printf("Unable to get response from");
				exit(1);
			}
		}
		else //Si se recibe una respuesta, se cancela la alarma
		{
			alarm(0);
			//Si la direccion es ADDRNOTFOUND, se imprime que no se encontro el host
			if (reqaddr.s_addr == ADDRNOTFOUND)
				printf("Host %s unknown by nameserver %s\n", argv[2], argv[1]);
			else
			{
				//Si se encontro la direccion, se imprime
				if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
					perror(" inet_ntop \n");
				printf("Address for %s is %s\n", argv[2], hostname);
			}
			break;
		}
	}

	if (n_retry == 0)
	{
		printf("Unable to get response from");
		printf(" %s after %d attempts.\n", argv[1], RETRIES);
	}
}
