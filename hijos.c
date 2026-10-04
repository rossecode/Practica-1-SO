#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/shm.h>
#include <sys/wait.h>

int *vecX, *vecY;
int shmidX, shmidY;

void validarArgumentos(int argc, char *argv[], int *x, int *y) {
    if (argc != 3) {
        printf("Argumentos incorrectos. Uso: %s <X> <Y>\n", argv[0]);
        exit(-1);
    }

    *x = atoi(argv[1]);
    *y = atoi(argv[2]);

    if (*x <= 0 || *y <= 0) {
        printf("Los argumentos X e Y deben ser mayores que cero\n");
        exit(-1);
    }
}

void reservarMemoria(int tamX, int tamY) {
    shmidX = shmget(IPC_PRIVATE, sizeof(int) * tamX, IPC_CREAT | 0666);
    if (shmidX < 0) {
        perror("Error al crear memoria compartida X");
        exit(-1);
    }

    vecX = (int *) shmat(shmidX, NULL, 0);
    if (vecX == (void *) -1) {
        perror("Error al vincular memoria compartida X");
        exit(-1);
    }

    shmidY = shmget(IPC_PRIVATE, sizeof(int) * tamY, IPC_CREAT | 0666);
    if (shmidY < 0) {
        perror("Error al crear memoria compartida Y");
        exit(-1);
    }

    vecY = (int *) shmat(shmidY, NULL, 0);
    if (vecY == (void *) -1) {
        perror("Error al vincular memoria compartida Y");
        exit(-1);
    }
}

void desvincularMemoria() {
    shmdt(vecX);
    shmdt(vecY);
}

void eliminarMemoria() {
    shmctl(shmidX, IPC_RMID, NULL);
    shmctl(shmidY, IPC_RMID, NULL);
}

void imprimirPadres(int x) {
    for (int i = 0; i < x; i++) {
        printf("%d", vecX[i]);
        if (i < x - 1) {
            printf(", ");
        }
    }
    printf("\n");
}

void crearHijosFinales(int x, int y) {
    pid_t pid;

    for (int i = 0; i < y; i++) {
        pid = fork();

        if (pid < 0) {
            perror("Error en fork");
            exit(-1);
        } else if (pid == 0) {
            printf("Soy el subhijo %d, mis padres son: ", getpid());
            imprimirPadres(x); 
            desvincularMemoria();
            exit(0);
        } else {
            vecY[i] = pid;
        }
    }
    for (int i = 0; i < y; i++) {
        wait(NULL);
    }
}

void crearCadenaPadres(int x, int y) {
    pid_t pid;
    pid_t superPadrePid = getpid(); 

    for (int i = 0; i < x; i++) {
        pid = fork();

        if (pid < 0) {
            perror("Error en fork");
            exit(-1);
        } else if (pid > 0) {
            waitpid(pid, NULL, 0);

            if (getpid() == superPadrePid) {
                return;
            }
            desvincularMemoria();
            exit(0);
        } else {
            vecX[i] = getpid();
            if (i == x - 1) {
                crearHijosFinales(x, y);
                desvincularMemoria();
                exit(0);
            }
        }
    }
}

void imprimirHijosFinales(int y) {
    printf("Soy el superpadre (%d): mis hijos finales son: ", getpid());

    for (int i = 0; i < y; i++) {
        printf("%d", vecY[i]);
        if (i < y - 1) {
            printf(", ");
        }
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int x, y;

    validarArgumentos(argc, argv, &x, &y);
    reservarMemoria(x, y);

    crearCadenaPadres(x, y);

    imprimirHijosFinales(y);

    desvincularMemoria();
    eliminarMemoria();

    return 0;
}