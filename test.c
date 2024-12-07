#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <utmp.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define MAX_STRING_LENGTH 516
#define MAX_USERS 1000

// Función para leer archivos de usuario (.plan y .project)
void leer_archivo_usuario(const char *home, const char *filename, char *buffer, size_t len)
{
	char filepath[256];
	snprintf(filepath, sizeof(filepath), "%s/%s", home, filename);

	int fd = open(filepath,0);
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

// Función para obtener la información de los usuarios
void obtener_usuarios(char usuarios[MAX_USERS][MAX_STRING_LENGTH], int *num_usuarios, char *usuario)
{
	struct passwd *pwd;
	struct utmp *ut;
	FILE *utmp_file;
	char terminal[50] = "N/A";
	char login_time[50] = "N/A";
	char where[50] = "N/A";
	char plan[256] = "N/A";
	char project[256] = "N/A";
	char buffer[MAX_STRING_LENGTH];
	char buffer_parcial[MAX_STRING_LENGTH];

	utmp_file = fopen(_PATH_UTMP, "r");
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
		if ((strstr(pwd->pw_gecos, usuario) == NULL) && (strcmp(usuario, "null") != 0))
        {
            continue;
        }
		{
			continue;
		}

		leer_archivo_usuario(pwd->pw_dir, ".plan", plan, sizeof(plan));
		leer_archivo_usuario(pwd->pw_dir, ".project", project, sizeof(project));

		snprintf(buffer, MAX_STRING_LENGTH,
				 "%s;%s;%s;%s;%s",
				 pwd->pw_name, pwd->pw_gecos, terminal, login_time, where);

		snprintf(buffer_parcial, sizeof(buffer_parcial),
				 "%s;%s;%s;%s----\r\n",
				 pwd->pw_dir, pwd->pw_shell, plan, project);

		// Copiar buffer y buffer_parcial en líneas consecutivas del array
		if (*num_usuarios + 1 < MAX_USERS)
		{
			strncpy(usuarios[*num_usuarios], buffer, MAX_STRING_LENGTH - 1);
			usuarios[*num_usuarios][MAX_STRING_LENGTH - 1] = '\0'; // Asegurar terminación
			(*num_usuarios)++;

			strncpy(usuarios[*num_usuarios], buffer_parcial, MAX_STRING_LENGTH - 1);
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

// Ejemplo de uso de la función
int main() {
    char usuarios[MAX_USERS][MAX_STRING_LENGTH];
    int num_usuarios = 0;

    obtener_usuarios(usuarios, &num_usuarios, "Jorge");

    printf("Usuarios conectados: %d\n", num_usuarios);

    for (int i = 0; i < num_usuarios; i++) {
        printf("%s", usuarios[i]);
    }

    return 0;
}
