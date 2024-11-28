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

#define PUERTO 13131
#define TAM_BUFFER 10

int main(int argc, char *argv[])
{
	int s; /* connected socket descriptor */
	struct addrinfo hints, *res;
	long timevar;					/* contains time returned by time() */
	struct sockaddr_in myaddr_in;	/* for local socket address */
	struct sockaddr_in servaddr_in; /* for server socket address */
	int addrlen, i, j, errcode;
	/* This example uses TAM_BUFFER byte messages. */
	char buf[TAM_BUFFER];

	if (argc != 3)
	{
		// Argumentos incorrectos para arrancar
		exit(1);
	}

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
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
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
	errcode = getaddrinfo(argv[1], NULL, &hints, &res);
	if (errcode != 0)
	{
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n", argv[0], argv[1]);
		exit(1);
	}
	else
	{
		// Si todo sale bien, se obtiene la IP del servidor
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
		perror(argv[0]);
		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
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
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}
	time(&timevar);

	printf("Connected to %s on port %u at %s", argv[1], ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

	// Envio de mensajes

	// Llenar el buffer con el mensaje que sea
	/*Uso de send
	 * send(s, buf, TAM_BUFFER, 0)
	 * s: socket descriptor
	 * buf: buffer con el mensaje
	 * TAM_BUFFER: tamaño del buffer
	 * 0: flags
	 */

	if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER)
	{
		fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
		fprintf(stderr, "on send number %d\n", i);
		exit(1);
	}

	// Cierre de la conexion
	if (shutdown(s, 1) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
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
			perror(argv[0]);
			fprintf(stderr, "%s: error reading result\n", argv[0]);
			exit(1);
		}
		// No tocar este while que se caga encima
		while (i < TAM_BUFFER) // Intentar completar tamaño
		{
			j = recv(s, &buf[i], TAM_BUFFER - i, 0);
			if (j == -1)
			{
				perror(argv[0]);
				fprintf(stderr, "%s: error reading result\n", argv[0]);
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
