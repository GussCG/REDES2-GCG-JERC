#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PUERTO 12345

int main() {
    int sockfd;
    struct sockaddr_in servidorAddr;
    char buffer[1024];
    int tamanoPaquete, tamanoVentana;
    FILE *archivo;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Error al crear el socket");
        exit(EXIT_FAILURE);
    }

    memset(&servidorAddr, 0, sizeof(servidorAddr));
    servidorAddr.sin_family = AF_INET;
    servidorAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servidorAddr.sin_port = htons(PUERTO);

    printf("Ingrese el tamaño del paquete: ");
    scanf("%d", &tamanoPaquete);
    printf("Ingrese el tamaño de la ventana: ");
    scanf("%d", &tamanoVentana);

    // Enviamos el tamaño del paquete y de la ventana al servidor
    sendto(sockfd, (const char *)&tamanoPaquete, sizeof(int), 0, (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));
    sendto(sockfd, (const char *)&tamanoVentana, sizeof(int), 0, (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));

    archivo = fopen("datos.txt", "rb");
    if (archivo == NULL) {
        perror("Error al abrir el archivo");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    int bytesLeidos;
    while ((bytesLeidos = fread(buffer, 1, tamanoPaquete, archivo)) > 0) {
        sendto(sockfd, buffer, bytesLeidos, 0, (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));

        int ackNum;
        recvfrom(sockfd, &ackNum, sizeof(int), 0, NULL, NULL);
        printf("ACK recibido: %d\n", ackNum);
    }

    fclose(archivo);
    close(sockfd);

    printf("Archivo enviado correctamente\n");
    return 0;
}
