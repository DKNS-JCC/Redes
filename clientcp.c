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
#define TAM_BUFFER 30
extern int errno;

#define ADDRNOTFOUND 0xffffffff /* value returned for unknown host */
#define RETRIES 5				/* number of times to retry before givin up */
#define BUFFERSIZE 1024			/* maximum size of packets to be received */
#define TIMEOUT 6
#define MAXHOST 512


void handler()
{
	printf("Alarma recibida \n");
}


	int funcionTCP(char usuario [], char host []){
		int s; /* connected socket descriptor */
		struct addrinfo hints, *res;
		long timevar;					/* contains time returned by time() */
		struct sockaddr_in myaddr_in;	/* for local socket address */
		struct sockaddr_in servaddr_in; /* for server socket address */
		int addrlen, i, j, errcode;
		/* This example uses TAM_BUFFER byte messages. */
		char buf[TAM_BUFFER];




		// Create the socket.
		/*Uso de socket
		* socket(AF_INET, SOCK_STREAM, 0)
		* AF_INET: protocolo de direccionamiento IPv4
		* SOCK_STREAM: protocolo de comunicacion TCP
		* 0: protocolo de comunicacion, 0 es el default para TCP
		*/
		s = socket(AF_INET, SOCK_STREAM, 0);
		if (s == -1)
		{
			perror("error argv0");//argv[0]
			fprintf(stderr, ": unable to create socket\n"); //aqui ponia %S argv[0] pero para que no de error de momento
			exit(1);
		}

		memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
		memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

		// Decir que es IPv4
		servaddr_in.sin_family = AF_INET;

		// Configurar hints para getaddrinfo
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;

		// Obtener EL IP del servidor
		/*Uso de getaddrinfo
		* getaddrinfo(argv[1], NULL, &hints, &res)
		* argv[1]: direccion del servidor
		* NULL: puerto del servidor, se configura mas adelante
		* &hints: estructura con informacion de la direccion
		* &res(ultado): estructura con informacion de la direccion
		*/
		errcode = getaddrinfo(host, NULL, &hints, &res); // Se obtiene la IP del host
		if (errcode != 0)
		{
			fprintf(stderr, ":No es posible resolver la IP de %s\n", host); //argv [0]
			exit(1);
		}
		else
		{
			servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
		}

		// Liberar memoria de res que ya no se necesita
		freeaddrinfo(res);

		// Traducir el puerto declarado
		servaddr_in.sin_port = htons(PUERTO);

		// Conectar al servidor
		/*
		Uso de connect
		* connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in))
		* s: socket descriptor
		* (const struct sockaddr *)&servaddr_in: direccion del servidor
		* sizeof(struct sockaddr_in): tamaño de la direccion
		*/
		if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
		{
			perror("error argv0");//argv[0]
			fprintf(stderr, ": unable to connect to remote\n"); //lo mismo argv[0]
			exit(1);
		}

		addrlen = sizeof(struct sockaddr_in);

		// Obtener la direccion del socket local
		/*
		Uso de getsockname
		* getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen)
		* s: socket descriptor
		* (struct sockaddr *)&myaddr_in: direccion del socket
		* &addrlen: tamaño de la direccion
		*/
		if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
		{
			perror("error argv0");//argv[0]
			fprintf(stderr, ": unable to read socket address\n"); //lo mismo
			exit(1);
		}
		time(&timevar);

		printf("Connected to %s on port %u at %s", host, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

		// AQUI SE LLENA EL BUFFER CON EL MENSAJE FINGER =======================================================================
		// Llenar el buffer con el mensaje que sea

		snprintf(buf, TAM_BUFFER, "%s|%s", usuario, host);

		/*Uso de send
		* send(s, buf, TAM_BUFFER, 0)
		* s: socket descriptor
		* buf: buffer con el mensaje
		* TAM_BUFFER: tamaño del buffer
		* 0: flags
		*/

		if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER)
		{
			fprintf(stderr, ": Connection aborted on error "); //argv[0]
			fprintf(stderr, "on send number %d\n", i);
			exit(1);
		}

		// Cierre de la conexion
		if (shutdown(s, 1) == -1)
		{
			perror("error argv0");//argv[0]
			fprintf(stderr, ": unable to shutdown socket\n"); //lo mismo
			exit(1);
		}

		// Recibir mensajes
		/*Uso de recv
		* recv(s, buf, TAM_BUFFER, 0)
		* s: socket descriptor
		* buf: buffer con el mensaje
		* TAM_BUFFER: tamaño del buffer
		* 0: flags
		*/
		while (i = recv(s, buf, TAM_BUFFER, 0))
		{
			if (i == -1)
			{
				perror("error argv0");//argv[0]
				fprintf(stderr, ": error reading result\n"); //lo mismo
				exit(1);
			}
			// No tocar este while que se caga encima
			while (i < TAM_BUFFER) // Intentar completar tamaño
			{
				j = recv(s, &buf[i], TAM_BUFFER - i, 0);
				if (j == -1)
				{
					perror("error argv0");
					fprintf(stderr, ": error reading result\n"); //lo mismo
					exit(1);
				}
				i += j;
			}
			// IMPRIMIR RECEPCION AQUI
			printf("Received %s\n", buf);
		}
		time(&timevar);
		// FIN
		printf("All done at %s", (char *)ctime(&timevar));
	}

	int funcionUDP(char usuario [], char host []){
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

		char buf[TAM_BUFFER];

		/* Create the socket. */
		s = socket(AF_INET, SOCK_DGRAM, 0);
		if (s == -1)
		{
			perror("error argv0");//argv[0]
			fprintf(stderr, ": unable to create socket\n"); //lo mismo
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
			perror("error argv0");//argv[0]	
			fprintf(stderr, ": unable to bind socket\n"); //lo mismo argv[0]
			exit(1);
		}
		addrlen = sizeof(struct sockaddr_in);

		// Obtener la direccion del socket local
		if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
		{
			perror("error argv0");//argv[0]
			fprintf(stderr, ": unable to read socket address\n"); //lo mismo
			exit(1);
		}
		time(&timevar);
		printf("Connected to %s on port %u at %s", host, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

		servaddr_in.sin_family = AF_INET;
		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;
		errcode = getaddrinfo(host, NULL, &hints, &res); //argv1 dnd host
		if (errcode != 0)
		{
			fprintf(stderr, ": No es posible resolver la IP de %s\n", host); //argv [0]
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
			fprintf(stderr, ": unable to register the SIGALRM signal\n"); //argv[0]
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
			snprintf(buf, TAM_BUFFER, "%s|%s", usuario, host);
			if (sendto(s, buf, strlen(buf), 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
			{
				perror("error argv0"); //argv[0]
				fprintf(stderr, ": unable to send request\n"); //lo mismo
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
					printf("Host %s unknown by nameserver %s\n", usuario, host); //ns si es usuario host o host usuario 
				else
				{
					//Si se encontro la direccion, se imprime
					if (inet_ntop(AF_INET, &reqaddr, hostname, MAXHOST) == NULL)
						perror(" inet_ntop \n");
					printf("Address for %s is %s\n", usuario, hostname); //argv[2] dnd usuario
				}
				break;
			}
		}

		if (n_retry == 0)
		{
			printf("Unable to get response from");
			printf(" %s after %d attempts.\n", host, RETRIES); //argv[1] dnd host
		}
	}



	int main(int argc, char *argv[])
{
	

	char usuario[50]; // Usuario a buscar
	char host[70] = "127.0.0.1";	  // Host al que se conecta

	// Verificar argumentos
	if (argc > 3)
	{
		fprintf(stderr, "Usage: %s TCP/UDP [usuario/nombrereal][usuario@host]\n", argv[0]);
		exit(1);
	}
	else if (argc == 3)
	{
		printf("argumentos SI son 3 y son argv[0] = %s, argv[1] %s argv[2] %s\n", argv[0], argv[1], argv[2]);
		
		if (strchr(argv[2], '@') == NULL) // Si no se encuentra el caracter @ es usuario o host
		{
			if (strchr(argv[2], '.') == NULL) // Si no se encuentra el caracter . es usuario
			{
				strcpy(usuario, argv[2]);
				strcpy(host, "127.0.0.1");
			}
			else
			{
				strcpy(host, argv[2]);
			}
		}

		else // Si se encuentra el caracter @ es usuario@host.es
		{
			strcpy(usuario, strtok(argv[2], "@"));
			strcpy(host, strtok(NULL, "@"));
		}
		printf("el usuario es %s y el host es %s\n", usuario, host);
		if(strcmp(argv[1], "TCP") == 0){
			funcionTCP(usuario, host);
		}
		else if(strcmp(argv[1], "UDP") == 0){
			funcionUDP(usuario, host);
		}
		else{
			fprintf(stderr, "Usage: %s TCP/UDP [usuario/nombrereal][usuario@host]\n", argv[0]);
			exit(1);
		}
	}
	else{

		strcpy(usuario, "nodesignado");
		strcpy(host, "127.0.0.1");


		if(strcmp(argv[1], "TCP") == 0){
			funcionTCP(usuario, host);
		}
		else if(strcmp(argv[1], "UDP") == 0){
			funcionUDP(usuario, host);
		}
		else{
			fprintf(stderr, "Usage: %s TCP/UDP [usuario/nombrereal][usuario@host]\n", argv[0]);
			exit(1);
		}

	}
}
