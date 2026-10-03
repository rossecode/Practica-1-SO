
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define TAM_BUFFER 1024

void validarArgumentos(int argc, char *argv[]) {

    if(argc != 3) {
        printf("Argumentos incorrectos\n");
        printf("Uso: %s <archivo> <tamaño>\n", argv[0]);
        exit(-1);
    }
}
int abrirOrigen(char *archivo) {
    int fd;
    fd = open(archivo, O_RDONLY);
    if(fd < 0) {
        perror("Fallo al abrir el archivo origen");
        exit(-1);
    }
    return fd;
}

void crearNombre(char *archivo, int numTrozo, char *destino) {
    sprintf(destino, "%s.h%02d", archivo, numTrozo);
}

int abrirDestino(char *archivo, int numTrozo) {
    char destino[256];
    int fd;

    crearNombre(archivo, numTrozo, destino);
    fd = creat(destino, 0644);

    if(fd < 0) {
        perror("Error al crear el fragmento");
        exit(-1);
    }
    return fd;
}

int tamArchivo(int fdOrigen) {
    int tam;

    tam = lseek(fdOrigen, 0, SEEK_END);

    if(tam < 0) {
        perror("Error al obtener el tamaño del archivo");
        exit(-1);
    }

    if(lseek(fdOrigen, 0, SEEK_SET) < 0) {
        perror("Error al volver al inicio del archivo");
        exit(-1);
    }
    return tam;
}
int calcularFragmentos(int fdOrigen, int tamBloque) {
    int tamTotal;

    tamTotal = tamArchivo(fdOrigen);

    return (tamTotal + tamBloque - 1) / tamBloque;
}
void procesoHijo(int fdLectura, char *archivo, int numTrozo) {
    char buffer[TAM_BUFFER];
    int fdDestino;
    int leidos;

    fdDestino = abrirDestino(archivo, numTrozo);

    while((leidos = read(fdLectura, buffer, TAM_BUFFER)) > 0) {

        if(write(fdDestino, buffer, leidos) != leidos) {
            perror("Error al escribir el fragmento");
            exit(-1);
        }
    }

    if(leidos < 0) {
        perror("Error al leer de la tuberia");
        exit(-1);
    }
    close(fdDestino);
    close(fdLectura);
    exit(0);
}


void enviarBloque(int fdOrigen, int fdEscritura, int tamBloque) {
    char buffer[TAM_BUFFER];

    int leidos;
    int porLeer;
    int aLeer;

    porLeer = tamBloque;

    while(porLeer > 0) {

        if(porLeer < TAM_BUFFER) {
            aLeer = porLeer;
        }
        else {
            aLeer = TAM_BUFFER;
        }
        leidos = read(fdOrigen, buffer, aLeer);

        if(leidos < 0) {
            perror("Error al leer el archivo");
            exit(-1);
        }
        if(leidos == 0) {
            break;
        }

        if(write(fdEscritura, buffer, leidos) != leidos) {
            perror("Error al escribir en la tuberia");
            exit(-1);
        }
        porLeer = porLeer - leidos;
    }
}


void crearBloque(int fdOrigen, char *archivo, int tamBloque, int numTrozo) {
    int tuberia[2];
    pid_t pid;

    if(pipe(tuberia) < 0) {
        perror("Fallo al crear tuberia");
        exit(-1);
    }
    pid = fork();

    if(pid < 0) {
        perror("Error en fork");
        exit(-1);
    }
    else if(pid == 0) {
        close(tuberia[1]);
        close(fdOrigen);
        procesoHijo(tuberia[0], archivo, numTrozo);
    }
    else {
        close(tuberia[0]);
        enviarBloque(fdOrigen, tuberia[1], tamBloque);
        close(tuberia[1]);
    }
}


int main(int argc, char *argv[]) {
    int fdOrigen, tamBloque,fragmentos, i;

    char *archivo;
    validarArgumentos(argc, argv);

    archivo = argv[1];
    tamBloque = atoi(argv[2]);

    if(tamBloque <= 0) {
        printf("El tamaño debe ser un número positivo y mayor que cero\n");
        exit(-1);
    }
    fdOrigen = abrirOrigen(archivo);
    fragmentos = calcularFragmentos(fdOrigen, tamBloque);

    for(i = 0; i < fragmentos; i++) {

        crearBloque(fdOrigen, archivo, tamBloque, i);
    }
    close(fdOrigen);
    for(i = 0; i < fragmentos; i++) {
        wait(NULL);
    }
    printf("Archivo dividido en %d fragmentos\n", fragmentos);

    return 0;
}