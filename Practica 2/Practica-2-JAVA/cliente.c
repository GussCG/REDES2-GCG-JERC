// Practica 2 REDES (SERVIDOR)
// Autores: Cerda Garcia Gustavo / Ramirez Carrillo Jose Emilio
// Grupo: 6CV4
// Descripción: Implementación del mecanismo de control de flujo de "Ventana deslizante" para la transmisión
// - El usuario definirá el tamaño de la ventana y el tamaño del paquete
// - Se implementara el mecanismo de control de error "Retroceder-N"

/*Librerias*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h> // Libreria para sockets en Windows
#include <stddef.h> // Definiciones de tipos de datos
#include <sys/types.h>

/*Definiciones*/
#define PUERTO 12345 // Puerto de comunicación

int main() {
    WSADATA wsa; // Estructura que contiene información sobre la implementación de Windows Sockets
    SOCKET sockfd; // Descriptor del socket
    struct sockaddr_in servidorAddr; // Estructura para almacenar la dirección del servidor
    char buffer[1024]; // Buffer para almacenar los paquetes a enviar
    int tamanoPaquete, tamanoVentana; // Tamaño del paquete y de la ventana
    FILE *archivo; // Archivo a enviar

    // Inicializar Winsock
    // WSAStartup() carga la implementación de Windows Sockets (Winsock) y devuelve información sobre la implementación
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Error al inicializar Winsock");
        return 1;
    }

    // Crear socket
    // socket() crea un socket y devuelve un descriptor que se utilizará en las llamadas subsiguientes
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("Error al crear el socket: %d", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    // Configurar dirección del servidor
    memset(&servidorAddr, 0, sizeof(servidorAddr)); // Se inicializa la estructura con ceros
    servidorAddr.sin_family = AF_INET; // Familia de direcciones
    servidorAddr.sin_addr.s_addr = inet_addr("127.0.0.1"); // Dirección IP del servidor
    servidorAddr.sin_port = htons(PUERTO); // Puerto de comunicación

    // Solicitar al usuario el tamaño del paquete y de la ventana
    printf("Ingrese el tamano del paquete: ");
    scanf("%d", &tamanoPaquete);
    printf("Ingrese el tamano de la ventana: ");
    scanf("%d", &tamanoVentana);

    // Enviar tamaño del paquete y de la ventana al servidor
    sendto(sockfd, (const char *)&tamanoPaquete, sizeof(int), 0,
           (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));
    sendto(sockfd, (const char *)&tamanoVentana, sizeof(int), 0,
           (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));
    
    // Abrir archivo a enviar
    archivo = fopen("datos.txt", "rb"); // Se abre el archivo en modo lectura binaria
    if (archivo == NULL) {
        printf("Error al abrir el archivo");
        closesocket(sockfd);
        WSACleanup();
        return 1;
    }

    // Leer archivo y enviar paquetes
    // Se envían los paquetes mientras no se haya llegado al final del archivo
    int bytesLeidos;
    while ((bytesLeidos = fread(buffer, 1, tamanoPaquete, archivo)) > 0){
        sendto(sockfd, buffer, bytesLeidos, 0, (const struct sockaddr *)&servidorAddr, sizeof(servidorAddr));
    }

    fclose(archivo);
    closesocket(sockfd);
    WSACleanup();

    printf("Archivo enviado correctamente\n");
    return 0;
}
