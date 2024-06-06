#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PUERTO 12345
#define TAMANO_PAQUETE 1024

typedef struct {
    struct sockaddr_in clienteAddr;
    socklen_t clienteAddrLen;
} Conexion;

void enviarACK(int sockfd, Conexion *conexion, int ackNum) {
    sendto(sockfd, &ackNum, sizeof(int), 0, (struct sockaddr *)&(conexion->clienteAddr), conexion->clienteAddrLen);
}

int main() {
    int sockfd;
    struct sockaddr_in servidorAddr, clienteAddr;
    socklen_t clienteAddrLen = sizeof(clienteAddr);
    char buffer[TAMANO_PAQUETE];
    FILE *archivo;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    memset(&servidorAddr, 0, sizeof(servidorAddr));
    servidorAddr.sin_family = AF_INET;
    servidorAddr.sin_addr.s_addr = INADDR_ANY;
    servidorAddr.sin_port = htons(PUERTO);

    if (bind(sockfd, (struct sockaddr *)&servidorAddr, sizeof(servidorAddr)) < 0) {
        perror("Error al vincular el socket");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Servidor iniciado en el puerto %d\n", PUERTO);

    Conexion conexion;
    conexion.clienteAddrLen = clienteAddrLen;

    int base = 0;

    while (1) {
        int bytesRecibidos = recvfrom(sockfd, buffer, TAMANO_PAQUETE, 0, (struct sockaddr *)&(clienteAddr), &clienteAddrLen);

        if (bytesRecibidos > 0) {
            conexion.clienteAddr = clienteAddr;
            printf("Paquete recibido: %d\n", base);
            if (base == *(int *)buffer) {
                archivo = fopen("datos_recibidos.txt", "ab");
                fwrite(buffer + sizeof(int), 1, bytesRecibidos - sizeof(int), archivo);
                fclose(archivo);
                enviarACK(sockfd, &conexion, base++);
            } else {
                enviarACK(sockfd, &conexion, base - 1);
            }
        }
    }

    close(sockfd);
    return 0;
}
