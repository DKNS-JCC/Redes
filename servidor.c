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
#include <pwd.h>
#include <utmp.h>

#define PUERTO 13131
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define BUFFERSIZE 1024			/* maximum size of packets to be received */
#define TAM_BUFFER 516
#define MAXHOST 128
#define MAX_USERS 200
#define MAX_STRING_LENGTH 516

extern int errno;

/*
 *			M A I N
 *
 *	This routine starts the server.  It forks, leaving the child
 *	to do all the work, so it does not have to be run in the
 *	background.  It sets up the sockets.  It
 *	will loop forever, until killed by a signal.
 *
 */

// Declarar struct de usuario
void serverTCP(int s, struct sockaddr_in peeraddr_in);
void serverUDP(int s, char *buffer, struct sockaddr_in clientaddr_in);
void errout(char *); /* declare error out routine */
int i = 0;
int FIN = 0; /* Para el cierre ordenado */
void finalizar() { FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
{

	int s_TCP, s_UDP; /* connected socket descriptor */
	int ls_TCP;		  /* listen socket descriptor */

	int cc; /* contains the number of bytes read */

	struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */

	struct sockaddr_in myaddr_in;	  /* for local socket address */
	struct sockaddr_in clientaddr_in; /* for peer socket address */
	int addrlen;

	fd_set readmask;
	int numfds, s_mayor;

	char buffer[BUFFERSIZE]; /* buffer for packets to be read into */

	struct sigaction vec;

	/* Create the listen socket. */
	ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
	if (ls_TCP == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
		exit(1);
	}
	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

	addrlen = sizeof(struct sockaddr_in);

	/* Set up address structure for the listen socket. */
	myaddr_in.sin_family = AF_INET;
	/* The server should listen on the wildcard address,
	 * rather than its own internet address.  This is
	 * generally good practice for servers, because on
	 * systems which are connected to more than one
	 * network at once will be able to have one server
	 * listening on all networks at once.  Even when the
	 * host is connected to only one network, this is good
	 * practice, because it makes the server program more
	 * portable.
	 */
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	myaddr_in.sin_port = htons(PUERTO);

	/* Bind the listen address to the socket. */
	if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
		exit(1);
	}
	/* Initiate the listen on the socket so remote users
	 * can connect.  The listen backlog is set to 5, which
	 * is the largest currently supported.
	 */
	if (listen(ls_TCP, 5) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
		exit(1);
	}

	/* Create the socket UDP. */
	s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
	if (s_UDP == -1)
	{
		perror(argv[0]);
		printf("%s: unable to create socket UDP\n", argv[0]);
		exit(1);
	}
	/* Bind the server's address to the socket. */
	if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		perror(argv[0]);
		printf("%s: unable to bind address UDP\n", argv[0]);
		exit(1);
	}

	/* Now, all the initialization of the server is
	 * complete, and any user errors will have already
	 * been detected.  Now we can fork the daemon and
	 * return to the user.  We need to do a setpgrp
	 * so that the daemon will no longer be associated
	 * with the user's control terminal.  This is done
	 * before the fork, so that the child will not be
	 * a process group leader.  Otherwise, if the child
	 * were to open a terminal, it would become associated
	 * with that terminal as its control terminal.  It is
	 * always best for the parent to do the setpgrp.
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

		/* Set SIGCLD to SIG_IGN, in order to prevent
		 * the accumulation of zombies as each child
		 * terminates.  This means the daemon does not
		 * have to make wait calls to clean them up.
		 */
		if (sigaction(SIGCHLD, &sa, NULL) == -1)
		{
			perror(" sigaction(SIGCHLD)");
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

			if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
			{
				if (errno == EINTR)
				{
					FIN = 1;
					close(ls_TCP);
					close(s_UDP);
					perror("\nFinalizando el servidor. Señal recibida en select\n ");
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
				} /* De TCP*/
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
	int num_usuarios;
	char usuarios[MAX_USERS][MAX_STRING_LENGTH];
	char num_usuarios_string[10];

	char *received_data = NULL; // Buffer dinámico para todo el contenido recibido
	size_t total_received = 0;	// Total de datos recibidos
	received_data = (char *)malloc(1);
	if (!received_data)
	{
		perror("Error al inicializar buffer");
		close(s);
		return;
	}

	int len, len1, status;
	struct hostent *hp; /* pointer to host info for remote host */
	long timevar;		/* contains time returned by time() */

	struct linger linger; /* allow a lingering, graceful close; */
						  /* used when setting SO_LINGER */

	/* Look up the host information for the remote host
	 * that we have connected with.  Its internet address
	 * was returned by the accept call, in the main
	 * daemon loop above.
	 */

	status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
						 hostname, MAXHOST, NULL, 0, 0);
	if (status)
	{
		/* The information is unavailable for the remote
		 * host.  Just format its internet address to be
		 * printed out in the logging information.  The
		 * address will be shown in "internet dot format".
		 */
		/* inet_ntop para interoperatividad con IPv6 */
		if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
			perror(" inet_ntop \n");
	}
	/* Log a startup message. */
	time(&timevar);
	/* The port number must be converted first to host byte
	 * order before printing.  On most hosts, this is not
	 * necessary, but the ntohs() call is included here so
	 * that this program could easily be ported to a host
	 * that does require it.
	 */
	printf("Startup from %s port %u at %s",
		   hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));

	/* Set the socket for a lingering, graceful close.
	 * This will cause a final close of this socket to wait until all of the
	 * data sent on it has been received by the remote host.
	 */
	linger.l_onoff = 1;
	linger.l_linger = 1;
	if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
				   sizeof(linger)) == -1)
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
	/* Bucle para recibir datos del cliente */
	while ((len = recv(s, buf, TAM_BUFFER, 0)) > 0)
	{
		printf("Fragmento recibido: %d bytes\n", len);

		// Redimensionar el buffer dinámico
		char *temp = realloc(received_data, total_received + len + 1);
		if (!temp)
		{
			perror("Error al redimensionar buffer dinámico");
			free(received_data);
			close(s);
			return;
		}
		received_data = temp;

		// Copiar los datos recibidos al buffer dinámico
		memcpy(received_data + total_received, buf, len);
		total_received += len;

		// Asegurar terminación nula
		received_data[total_received] = '\0';

		reqcnt++; // Incrementar contador de solicitudes
	}

	sleep(1);

	if (len == -1)
	{
		perror("Error en recv");
	}
	else if (len == 0)
	{
		printf("El cliente cerró la conexión.\n");
	}

	char usuario[50];
	strncpy(usuario, received_data, sizeof(usuario));
	obtener_usuarios(usuarios, &num_usuarios, usuario, 0);

	/* Respuesta al cliente con el número de usuarios encontrados*/
	snprintf(num_usuarios_string, sizeof(num_usuarios_string), "%d", num_usuarios);
	if (send(s, num_usuarios_string, strlen(num_usuarios_string), 0) == -1)
	{
		perror("Error al enviar el número de usuarios");
		close(s);
		return;
	}

	/*Ir enviando linea a linea el contenido del array*/
	int i = 0;
	for (i = 0; i < num_usuarios; i++)
	{
		if (send(s, usuarios[i], strlen(usuarios[i]), 0) == -1)
		{
			perror("Error al enviar datos de usuario");
			close(s);
			return;
		}

		// Enviar un delimitador para indicar el fin de cada usuario
		const char delimitador[] = "\r\n";
		if (send(s, delimitador, strlen(delimitador), 0) == -1)
		{
			perror("Error al enviar delimitador");
			close(s);
			return;
		}
	}

	/* Cerrar conexión */
	close(s);

	/* Mostrar el contenido recibido */
	printf("Datos completos recibidos de %s:\n%s\n", hostname, received_data);
	printf("Total recibido: %zu bytes, %d solicitudes\n", total_received, reqcnt);

	/* Liberar memoria dinámica */
	free(received_data);

	/* Log de finalización */
	time(&timevar);
	printf("Conexión completada %s, %d solicitudes, en %s\n", hostname, reqcnt, ctime(&timevar));
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
	struct in_addr reqaddr; /* Para la dirección solicitada */
	int nc, errcode;
	socklen_t addrlen = sizeof(clientaddr_in);
	int num_usuarios;
	char usuarios[MAX_USERS][MAX_STRING_LENGTH];
	char num_usuarios_string[10];

	printf("Esperando datos UDP...\n");

	// Recibir datos del cliente
	ssize_t received_len = recvfrom(s, buffer, TAM_BUFFER, 0, (struct sockaddr *)&clientaddr_in, &addrlen);
	if (received_len == -1)
	{
		perror("Error al recibir datos");
		return;
	}

	// Asegurar terminación nula si se trata de datos de texto
	if (received_len < TAM_BUFFER)
	{
		buffer[received_len] = '\0';
	}

	// Mostrar la información del cliente y los datos recibidos
	char client_ip[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &clientaddr_in.sin_addr, client_ip, INET_ADDRSTRLEN);
	printf("Mensaje recibido de %s:%d\n", client_ip, ntohs(clientaddr_in.sin_port));
	printf("Datos recibidos (%zd bytes): %s\n", received_len, buffer);

	char usuario[50];
	strncpy(usuario, buffer, 50);

	obtener_usuarios(usuarios, &num_usuarios, usuario, 1);

	// Registrar en el archivo de log
	char descripcion[128];
	snprintf(descripcion, sizeof(descripcion), "Petición recibida por UDP del cliente %s:%d", client_ip, ntohs(clientaddr_in.sin_port));

	registrar_evento("Comunicación iniciada", "Cliente", client_ip, "UDP", ntohs(clientaddr_in.sin_port), NULL, NULL);
	registrar_evento("Orden recibida", "Cliente", client_ip, "UDP", ntohs(clientaddr_in.sin_port), buffer, NULL);

	char respuesta[512] = "Usuarios encontrados:\n";
	for (int i = 0; i < num_usuarios; i++)
	{
		strncat(respuesta, usuarios[i], sizeof(respuesta) - strlen(respuesta) - 1);
		strncat(respuesta, "\n", sizeof(respuesta) - strlen(respuesta) - 1);
	}
	registrar_evento("Respuesta enviada", "Cliente", client_ip, "UDP", ntohs(clientaddr_in.sin_port), NULL, respuesta);

	// Resolver el nombre recibido (si aplica)
	struct addrinfo hints, *res;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/*
	errcode = getaddrinfo(buffer, NULL, &hints, &res);
	if (errcode != 0) {
		printf("No se pudo resolver '%s'.\n", buffer);
		reqaddr.s_addr = ADDRNOTFOUND;
	} else {
		reqaddr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
		freeaddrinfo(res);
	}
	*/
	// Enviar la dirección resuelta al cliente

	// campos de sendto: socket, mensaje, longitud del mensaje, flags, dirección del cliente, longitud de la dirección

	printf("numUsuarios: %d\n", num_usuarios);

	snprintf(num_usuarios_string, sizeof(num_usuarios_string), "%d", num_usuarios);
	nc = sendto(s, num_usuarios_string, sizeof(num_usuarios_string), 0, (struct sockaddr *)&clientaddr_in, addrlen);
	if (nc == -1)
	{
		// p
		perror("Error al enviar respuesta");
		printf("No se pudo enviar la respuesta al cliente con el numero de usuarios.\n");
		return;
	}

	while (i < num_usuarios)
	{

		nc = sendto(s, usuarios[i], strlen(usuarios[i]), 0, (struct sockaddr *)&clientaddr_in, addrlen);
		if (nc == -1)
		{
			perror("Error al enviar respuesta");
			printf("No se pudo enviar la respuesta al cliente con cada usuario.\n");
			return;
		}
		i++;
	}

	printf("Respuesta enviada al cliente.\n");
}

// Función para leer archivos de usuario (.plan y .project)
void leer_archivo_usuario(const char *home, const char *filename, char *buffer, size_t len)
{
	char filepath[256];
	snprintf(filepath, sizeof(filepath), "%s/%s", home, filename);

	int fd = open(filepath, 0);
	if (fd < 0)
	{
		strncpy(buffer, "N/A", len);
		return;
	}

	ssize_t bytes_read = read(fd, buffer, len - 1);
	if (bytes_read > 0)
	{
		buffer[bytes_read] = '\0';
	}
	else
	{
		strncpy(buffer, "N/A", len);
	}
	close(fd);
}

void registrar_evento(const char *descripcion, const char *host, const char *ip, const char *protocolo, int puerto, const char *orden, const char *respuesta)
{
	FILE *log_file = fopen("registro.log", "a"); // Modo de edición
	if (!log_file)
	{
		perror("Error al abrir el archivo de log");
		return;
	}

	// Obtener fecha y hora actuales
	time_t now = time(NULL);
	char fecha_hora[50];
	strftime(fecha_hora, sizeof(fecha_hora), "%Y-%m-%d %H:%M:%S", localtime(&now)); // Formato: YYYY-MM-DD HH:MM:SS

	fprintf(log_file, "Fecha y hora: %s\n", fecha_hora);
	fprintf(log_file, "Descripción: %s\n", descripcion);
	fprintf(log_file, "Comunicación realizada: Host=%s, IP=%s, Protocolo=%s, Puerto=%d\n", host, ip, protocolo, puerto);
	if (orden)
	{
		fprintf(log_file, "Orden recibida: %s\n", orden);
	}
	if (respuesta)
	{
		fprintf(log_file, "Respuesta enviada: %s", respuesta);
	}
	fprintf(log_file, "\n----------------------------------------\n");

	fclose(log_file);
}

// Función para obtener la información de los usuarios
void obtener_usuarios(char usuarios[MAX_USERS][MAX_STRING_LENGTH], int *num_usuarios, char *usuario, int j)
{
	struct passwd *pwd;
	struct utmp *ut;
	FILE *utmp_file;
	char terminal[50] = "N/A";
	char login_time[50] = "N/A";
	char where[50] = "N/A";
	char plan[256] = "N/A";
	char project[256] = "N/A";
	char buffer[516];

	sleep(1);
	utmp_file = fopen("/var/run/utmp", "r");
	if (!utmp_file)
	{
		perror("Error abriendo /var/run/utmp");
		exit(EXIT_FAILURE);
	}

	*num_usuarios = 0;
	memset(usuarios, 0, sizeof(char) * MAX_USERS * MAX_STRING_LENGTH);

	while ((pwd = getpwent()) != NULL)
	{
		strcpy(terminal, "N/A");
		strcpy(login_time, "N/A");
		strcpy(where, "N/A");
		strcpy(plan, "N/A");
		strcpy(project, "N/A");

		setutent();
		while ((ut = getutent()) != NULL)
		{
			if (ut->ut_type == USER_PROCESS && strcmp(ut->ut_user, pwd->pw_name) == 0)
			{
				snprintf(terminal, sizeof(terminal), "%s", ut->ut_line);
				snprintf(where, sizeof(where), "%s", ut->ut_host);
				strftime(login_time, sizeof(login_time), "%Y-%m-%d %H:%M:%S", localtime(&ut->ut_tv.tv_sec));
				break;
			}
		}
		endutent();

		if (strcmp(login_time, "N/A") == 0)
		{
			continue;
		}

		int i = 0;
		char *NAME[100];
		char *LOGIN[100];
		strcpy(NAME, pwd->pw_gecos);
		strcpy(LOGIN, usuario);

		// Separar en palabras
		char *NAME2 = strtok(NAME, " ");
		char *LOGIN2 = strtok(LOGIN, " ");

		// Convertir a minúsculas
		for (i = 0; i < strlen(NAME2); i++)
		{
			NAME2[i] = tolower(NAME2[i]);
		}

		for (i = 0; i < strlen(LOGIN2); i++)
		{
			LOGIN2[i] = tolower(LOGIN2[i]);
		}

		// Asegurarse de que no haya saltos de línea en las subcadenas
		if (j == 0)
		{
			strcat(NAME2, "\r\n");
		}

		if ((strcmp(NAME2, LOGIN2) != 0) && (strcmp(LOGIN2, "null\r\n") != 0))
		{
			continue;
		}

		leer_archivo_usuario(pwd->pw_dir, ".plan", plan, sizeof(plan));
		leer_archivo_usuario(pwd->pw_dir, ".project", project, sizeof(project));

		snprintf(buffer, MAX_STRING_LENGTH,
				 "%s;%s;%s;%s;%s;%s;%s;%s;%s\r\n",
				 pwd->pw_name, pwd->pw_gecos, terminal, login_time, where, pwd->pw_dir, pwd->pw_shell, plan, project);

		// Copiar buffer y buffer_parcial en líneas consecutivas del array
		if (*num_usuarios + 1 < MAX_USERS)
		{
			strncpy(usuarios[*num_usuarios], buffer, MAX_STRING_LENGTH - 1);
			usuarios[*num_usuarios][MAX_STRING_LENGTH - 1] = '\0'; // Asegurar terminación
			(*num_usuarios)++;
		}
		else
		{
			fprintf(stderr, "Error: Número máximo de usuarios excedido.\n");
			break;
		}
	}
	fclose(utmp_file);
	endpwent();
}
