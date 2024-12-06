/*
 *			C L I E N T C P
 *
 *	This is an example program that demonstrates the use of
 *	stream sockets as an IPC mechanism.  This contains the client,
 *	and is intended to operate in conjunction with the server
 *	program.  Together, these two programs
 *	demonstrate many of the features of sockets, as well as good
 *	conventions for using these features.
 *
 *
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <sys/errno.h>
#include <signal.h>

#define PUERTO 13131
#define TAM_BUFFER 12

extern int errno;

#define ADDRNOTFOUND 0xffffffff /* value returned for unknown host */
#define RETRIES 5				/* number of times to retry before givin up */
#define BUFFERSIZE 1024			/* maximum size of packets to be received */
#define TIMEOUT 6
#define MAXHOST 512

/*
 *			M A I N
 *
 *	This routine is the client which request service from the remote.
 *	It creates a connection, sends a number of
 *	requests, shuts down the connection in one direction to signal the
 *	server about the end of data, and then receives all of the responses.
 *	Status will be written to stdout.
 *
 *	The name of the system to which the requests will be sent is given
 *	as a parameter to the command.
 */
void funcionTCP(char usuario[], char host[]);
void funcionUDP(char usuario[], char host[]);

int main(int argc, char *argv[])
{
	char usuario[50] = {0};
	char host[50] = {0};

	if (argc < 2 || argc > 3)
	{
		perror("Uso: ./client <TCP/UDP> <usuario/usuario@host/host>");
		exit(1);
	}

	if (argc == 2)
	{
		if (strcmp(argv[1], "TCP") == 0)
		{
			printf("ARGUMENTOS SON 2\n");
			funcionTCP("null", "localhost");
		}
		else if (strcmp(argv[1], "UDP") == 0)
		{
			funcionUDP("null", "localhost");
		}
		else
		{
			perror("Uso: ./client <TCP/UDP> <usuario/usuario@host/host>");
			exit(1);
		}
	}
	else if (argc == 3)
	{

		char *at_position = strchr(argv[2], '@');
		if (at_position)
		{
			// Caso usuario@host
			size_t usuario_len = at_position - argv[2];
			if (usuario_len < sizeof(usuario))
			{
				strncpy(usuario, argv[2], usuario_len);
				usuario[usuario_len] = '\0'; // Asegura terminación nula
			}
			else
			{
				fprintf(stderr, "Error: Usuario demasiado largo.\n");
				exit(1);
			}

			if (strlen(at_position + 1) < sizeof(host))
			{
				strcpy(host, at_position + 1);
			}
			else
			{
				fprintf(stderr, "Error: Host demasiado largo.\n");
				exit(1);
			}

			printf("Usuario: %s\n", usuario);
			printf("Host: %s\n", host);
		}
		else
		{
			// Caso sin '@'
			if (strchr(argv[2], '.'))
			{
				// Si es un host
				if (strlen(argv[2]) < sizeof(host))
				{
					strcpy(host, argv[2]);
					strcpy(usuario, "null");
				}
				else
				{
					fprintf(stderr, "Error: Host demasiado largo.\n");
					exit(1);
				}
			}
			else
			{
				// Si es un usuario
				if (strlen(argv[2]) < sizeof(usuario))
				{
					strcpy(usuario, argv[2]);
					strcpy(host, "localhost");
				}
				else
				{
					fprintf(stderr, "Error: Usuario demasiado largo.\n");
					exit(1);
				}
			}
		}

		if (strcmp(argv[1], "TCP") == 0)
		{
			funcionTCP(usuario, host);
		}
		else if (strcmp(argv[1], "UDP") == 0)
		{
			funcionUDP(usuario, host);
		}
		else
		{
			perror("Uso: ./client <TCP/UDP> <usuario/usuario@host/host>");
			exit(1);
		}
	}
}
void funcionTCP(char usuario[], char host[])
{

	char buf[TAM_BUFFER]; /* buffer for messages */
	int s;				  /* connected socket descriptor */
	struct addrinfo hints, *res;
	long timevar;					/* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in; /* for server socket address */
	int addrlen, i, j, errcode;

	/* Create the socket. */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror("clientTCP");
		fprintf(stderr, "unable to create socket\n");
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;

	/* Get the host information for the hostname that the
	 * user passed in. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo(host, NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				"clientTCP", host);
		exit(1);
	}
	else
	{
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);

	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

	/* Try to connect to the remote server at the address
	 * which was just built into peeraddr.
	 */
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror("clientTCP");
		fprintf(stderr, "%s: unable to connect to remote\n", "clientTCP");
		exit(1);
	}
	/* Since the connect call assigns a free address
	 * to the local end of this connection, let's use
	 * getsockname to see what it assigned.  Note that
	 * addrlen needs to be passed in as a pointer,
	 * because getsockname returns the actual length
	 * of the address.
	 */
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		perror("clientTCP");
		fprintf(stderr, "%s: unable to read socket address\n", "clientTCP");
		exit(1);
	}

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("TCP_Client Connected to %s on port %u at %s",
		   host, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

	if (usuario != "null")
	{
		strcat(usuario, "\r\n");
		/* Send the request to the server followed by \r\n */
		if (send(s, usuario, strlen(usuario), 0) == -1)
		{
			perror("clientTCP");
			fprintf(stderr, "%s: unable to send request\n", "clientTCP");
			exit(1);
		}
		
	}
	else 
	{
		char clrf [] = "\r\n";
		if (send(s, clrf, strlen(clrf), 0) == -1)
		{
			perror("clientTCP");
			fprintf(stderr, "%s: unable to send request\n", "clientTCP");
			exit(1);
		}
	}

	/* Now, shutdown the connection for further sends.
	 * This will cause the server to receive an end-of-file
	 * condition after it has received all the requests that
	 * have just been sent, indicating that we will not be
	 * sending any further requests.
	 */
	if (shutdown(s, 1) == -1)
	{
		perror("clientTCP");
		fprintf(stderr, "%s: unable to shutdown socket\n", "clientTCP");
		exit(1);
	}

	/* Now, start receiving all of the replys from the server.
	 * This loop will terminate when the recv returns zero,
	 * which is an end-of-file condition.  This will happen
	 * after the server has sent all of its replies, and closed
	 * its end of the connection.
	 */
	while (i = recv(s, buf, TAM_BUFFER, 0))
	{
		if (i == -1)
		{
			perror("clientTCP");
			fprintf(stderr, "%s: error reading result\n", "clientTCP");
			exit(1);
		}
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
		 * the begining of the next reply.
		 */
		while (i < TAM_BUFFER)
		{
			j = recv(s, &buf[i], TAM_BUFFER - i, 0);
			if (j == -1)
			{
				perror("clientTCP");
				fprintf(stderr, "%s: error reading result\n", "clientTCP");
				exit(1);
			}
			i += j;
		}
		/* Print out message indicating the identity of this reply. */
		printf("Received result number %d\n", *buf);
	}

	/* Print message indicating completion of task. */
	time(&timevar);
	printf("All done at %s", (char *)ctime(&timevar));
}

void handler()
{
	printf("Alarma recibida \n");
}

void funcionUDP(char usuario[], char host[])
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

	/* Create the socket. */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
	{
		perror("clientUDP");
		fprintf(stderr, "%s: unable to create socket\n", "clientUDP");
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
	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror("clientUDP");
		fprintf(stderr, "%s: unable to bind socket\n", "clientUDP");
		exit(1);
	}
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		perror("clientUDP");
		fprintf(stderr, "%s: unable to read socket address\n", "clientUDP");
		exit(1);
	}

	/* Print out a startup message for the user. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("Connected to %s on port %u at %s", host, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
	/* Get the host information for the server's hostname that the
	 * user passed in.
	 */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo(host, NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
				"clientUDP", host);
		exit(1);
	}
	else
	{
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);
	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);

	/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	vec.sa_handler = (void *)handler;
	vec.sa_flags = 0;
	if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1)
	{
		perror(" sigaction(SIGALRM)");
		fprintf(stderr, "%s: unable to register the SIGALRM signal\n", "clientUDP");
		exit(1);
	}

	n_retry = RETRIES;

	while (n_retry > 0) {

        // Enviar el mensaje al servidor
        if (sendto(s, usuario, strlen(usuario), 0, (struct sockaddr *)&servaddr_in, sizeof(servaddr_in)) == -1) {
            perror("clientUDP");
            close(s);
            exit(1);
        }

        // Configurar temporizador para evitar bloqueo
        alarm(TIMEOUT);

        // Esperar respuesta
        if (recvfrom(s, &reqaddr, sizeof(struct in_addr), 0, (struct sockaddr *)&servaddr_in, &addrlen) == -1) {
            if (errno == EINTR) {  // Timeout ocurrió
                printf("Intento %d fallido, reintentando...\n", RETRIES - n_retry + 1);
                n_retry--;
            } else {
                perror("Error en recvfrom");
                close(s);
                exit(1);
            }
        } else {
            alarm(0);  // Cancelar la alarma
            if (reqaddr.s_addr == ADDRNOTFOUND) {
                printf("Host %s desconocido.\n", host);
            } else {
                // Mostrar la respuesta
                if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL) {
                    perror("inet_ntop");
                } else {
                    printf("Dirección de %s: %s\n", host, hostname);
                }
            }
            break;
        }
    }

    // Si se agotaron los intentos
    if (n_retry == 0) {
        printf("No se pudo obtener respuesta después de %d intentos.\n", RETRIES);
    }

    close(s);
}
