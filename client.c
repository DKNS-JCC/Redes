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
#include <asm-generic/fcntl.h>

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
	const char *filename = "registro.txt";
	FILE *file = fopen(filename, "a");
	if (file == NULL)
	{
		perror("Error abriendo el archivo");
		return EXIT_FAILURE;
	}
	int fd = fileno(file);
	if (fd == -1)
	{
		perror("Error obteniendo el descriptor de archivo");
		fclose(file);
		return EXIT_FAILURE;
	}
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
		{ // Comprobar si hay algo delante de @
			// Caso usuario@host
			size_t usuario_len = at_position - argv[2];

			if (usuario_len < sizeof(usuario))
			{
				strncpy(usuario, argv[2], usuario_len);
				usuario[usuario_len] = '\0'; // Asegura terminación nula
			}
			if (usuario_len == 0)
			{
				strcpy(usuario, "null");
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
		char clrf[] = "null\r\n";
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
	if (shutdown(s, SHUT_WR) == -1)
	{
		perror("clientTCP");
		fprintf(stderr, "%s: unable to shutdown socket\n", "clientTCP");
		exit(1);
	}

	/* Now, start receiving all of the replys from the server.
	 * This loop will terminate when the recv returns zero,
	 * which is an end-of-file condition. This will happen
	 * after the server has sent all of its replies, and closed
	 * its end of the connection.
	 */

	size_t total_received = 0;
	char *received_data = NULL;
	ssize_t bytes_received;

	while ((bytes_received = recv(s, buf, TAM_BUFFER - 1, 0)) > 0)
	{
		buf[bytes_received] = '\0'; // Null-terminate the buffer

		// Resize buffer to hold new data
		char *temp = realloc(received_data, total_received + bytes_received + 1);
		if (!temp)
		{
			perror("realloc");
			free(received_data);
			close(s);
			exit(1);
		}
		received_data = temp;

		// Copy new data into the buffer
		memcpy(received_data + total_received, buf, bytes_received);
		total_received += bytes_received;
		received_data[total_received] = '\0';
	}

	if (bytes_received == -1)
	{
		perror("recv");
		free(received_data);
		close(s);
		exit(1);
	}

	/* Print the complete received data */
	printf("Received data from %s:\n%s\n", host, received_data);
	formatear_cadena(received_data);
	printf("Total bytes received: %zu\n", total_received);

	/* Clean up */
	free(received_data);
	time(&timevar);
	printf("\nAll done at %s", ctime(&timevar));

	/* Close the socket */
	close(s);
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
	char buffer[BUFFERSIZE];
	int num_usuarios = -1;
	char num_usuarios_string[10];
	int j = 0;

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

	while (n_retry > 0)
	{
		// Enviar el mensaje al servidor
		if (sendto(s, usuario, strlen(usuario), 0, (struct sockaddr *)&servaddr_in, sizeof(servaddr_in)) == -1)
		{
			perror("clientUDP");
			close(s);
			exit(1);
		}

		// Configurar temporizador para evitar bloqueo
		alarm(TIMEOUT);

		// Esperar respuesta
		// campos de recvfrom: socket, mensaje, longitud del mensaje, flags, dirección del cliente, longitud de la dirección
		if (num_usuarios == -1)
		{
			if (recvfrom(s, &num_usuarios_string, sizeof(num_usuarios_string), 0, (struct sockaddr *)&servaddr_in, &addrlen) == -1)
			{
				if (errno == EINTR)
				{ // Timeout ocurrió
					printf("Intento %d fallido, reintentando...\n", RETRIES - n_retry + 1);
					n_retry--;
				}
				else
				{
					perror("Error en recvfrom");
					close(s);
					exit(1);
				}
			}
			else
			{
				num_usuarios = atoi(num_usuarios_string);
				alarm(0); // Cancelar la alarma
				n_retry = RETRIES;
				printf("Usuarios encontrados recibidos en cliente: %d\n", num_usuarios);
			}
		}

		if (num_usuarios != -1)
		{
			alarm(TIMEOUT);
			while (j < num_usuarios)
			{
				memset(buffer, 0, BUFFERSIZE);
				if (recvfrom(s, &buffer, BUFFERSIZE - 1, 0, (struct sockaddr *)&servaddr_in, &addrlen) == -1)
				{
					if (errno == EINTR)
					{ // Timeout ocurrió
						printf("Intento %d fallido, reintentando...\n", RETRIES - n_retry + 1);
						n_retry--;
					}
					else
					{
						perror("Error en recvfrom");
						close(s);
						exit(1);
					}
				}
				else
				{
					alarm(0); // Cancelar la alarma
					n_retry = RETRIES;
					buffer[BUFFERSIZE - 1] = '\0';
					formatear_cadena(buffer);
					j++;
				}
			}
			if (j == num_usuarios)
			{
				break;
			}
		}
	}

	// Si se agotaron los intentos
	if (n_retry == 0)
	{
		printf("No se pudo obtener respuesta después de %d intentos.\n", RETRIES);
	}

	close(s);
}

void formatear_cadena(char *cadena) {
    char user[100], full_name[100], tty[50], login_time[100];
    char home_dir[100], where[100], shell[100], plan[256], project[256];
    char idle_time[100], mail_status[50], messages_status[50];

    // Dividir la cadena de entrada por los delimitadores ";" con strtok
    char *token = strtok(cadena, ";");
    strcpy(user, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(full_name, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(tty, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(login_time, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(where, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(home_dir, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(shell, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(plan, token ? token : "No Plan.");

    token = strtok(NULL, ";");
    strcpy(project, token ? token : "No Project.");

    token = strtok(NULL, ";");
    strcpy(idle_time, token ? token : "N/A");

    token = strtok(NULL, ";");
    strcpy(mail_status, token ? token : "No mail.");

    token = strtok(NULL, ";");
    strcpy(messages_status, token ? token : "messages off");

    // Escritura en archivo
    const char *filename = "registro.txt";
    FILE *file = fopen(filename, "a");
    if (file == NULL) {
        perror("Error abriendo el archivo");
        exit(EXIT_FAILURE);
    }

    int fd = fileno(file);
    if (fd == -1) {
        perror("Error obteniendo el descriptor de archivo");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    if (flock(fd, LOCK_EX) == -1) {
        perror("Error bloqueando el archivo");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fprintf(file, "========================================\n");
    fprintf(file, "%s\n", user);
    fprintf(file, "%s\n", full_name);
    fprintf(file, "%s\t\t", tty);
    fprintf(file, "%s\n", login_time);
    fprintf(file, "%s\n", idle_time);
    fprintf(file, "%s\t\t%s\n", messages_status, mail_status);
    fprintf(file, "%s\t\t%s\n", home_dir, shell);
    fprintf(file, "%s\n", plan);
    fprintf(file, "%s\n", project);
    fprintf(file, "========================================\n");

    if (flock(fd, LOCK_UN) == -1) {
        perror("Error desbloqueando el archivo");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    fclose(file);
}
