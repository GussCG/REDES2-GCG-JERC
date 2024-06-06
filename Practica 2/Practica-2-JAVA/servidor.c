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
#include <unistd.h>
#include <winsock2.h> // Libreria para sockets en Windows

/*Definiciones*/
#define PUERTO 12345 // Puerto de comunicación
#define TAMANO_PAQUETE 1024 // Tamaño del paquete

// Encabezados

// Función para retroceder N paquetes
// Descripción: Se retrocede N paquetes y se envía una confirmación al cliente
// Parámetros:
// - sockfd: Descriptor del socket
// - clienteAddr: Estructura con la dirección del cliente
// - clienteAddrLen: Longitud de la dirección del cliente
// - buffer: Buffer para almacenar los paquetes recibidos
// - bytesRecibidos: Número de bytes recibidos
// - archivo: Archivo para escritura
// Retorno: void
void retrocederNPaquetes(int sockfd, struct sockaddr_in * clienteAddr, int * clienteAddrLen, char * buffer, int * bytesRecibidos, FILE * archivo);

// Estructura para almacenar la información de cada cliente
// - clienteAddr: Dirección del cliente
// - tamanoPaquete: Tamaño del paquete
// - tamanoVentana: Tamaño de la ventana
typedef struct {
    struct sockaddr_in clienteAddr;
    int tamanoPaquete;
    int tamanoVentana;
} ClienteInfo;

int main() {
    WSADATA wsa; // Estructura que contiene información sobre la implementación de Windows Sockets
    SOCKET sockfd; // Descriptor del socket
    struct sockaddr_in servidorAddr, clienteAddr; // Estructuras para almacenar la dirección del servidor y del cliente
    int clienteAddrLen = sizeof(clienteAddr); // Longitud de la dirección del cliente
    char buffer[TAMANO_PAQUETE]; // Buffer para almacenar los paquetes recibidos
    FILE *archivo; // Archivo para escritura

    ClienteInfo clientes[10]; // Estructura para almacenar la información de los clientes
    int numClientes = 0; // Número de clientes conectados

    // Inicializar Winsock
    // WSAStartup() carga la implementación de Windows Sockets (Winsock) y devuelve información sobre la implementación
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Error al inicializar Winsock"); 
        return 1;
    }

    printf("Servidor iniciado en el puerto %d\n", PUERTO);

    // Crear socket
    // socket() crea un socket y devuelve un descriptor que se utilizará en las llamadas subsiguientes
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("Error al crear el socket: %d", WSAGetLastError());
        return 1;
    }

    // Configurar dirección del servidor
    // Se inicializa la estructura con ceros y se asignan los valores necesarios
    memset(&servidorAddr, 0, sizeof(servidorAddr)); 
    servidorAddr.sin_family = AF_INET; // Familia de direcciones
    servidorAddr.sin_addr.s_addr = INADDR_ANY; // Dirección IP del servidor
    servidorAddr.sin_port = htons(PUERTO); // Puerto de comunicación

    // Vincular socket a la dirección del servidor
    // bind() asocia el socket con la dirección del servidor
    if (bind(sockfd, (struct sockaddr *)&servidorAddr, sizeof(servidorAddr)) == SOCKET_ERROR) {
        printf("Error al vincular el socket: %d", WSAGetLastError());
        closesocket(sockfd); // Cerrar socket
        WSACleanup(); // Limpiar Winsock
        return 1;
    }

    while (1){
        int bytesRecibidos = recvfrom(sockfd, buffer, TAMANO_PAQUETE, 0, (struct sockaddr *)&clienteAddr, &clienteAddrLen);

        if (bytesRecibidos > 0){
            int clienteExistente = -1;
            for (int i = 0; i < numClientes; i++) {
                if (memcmp(&clientes[i].clienteAddr, &clienteAddr, sizeof(clienteAddr)) == 0) {
                    clienteExistente = i;
                    break;
                }
            }
            
            if (clienteExistente == -1){
                if (numClientes >= 10) {
                    printf("No se pueden aceptar más clientes\n");
                    continue;
                }
                clientes[numClientes].clienteAddr = clienteAddr;
                numClientes++;
                clienteExistente = numClientes - 1;
            }

            int tamanoPaquete, tamanoVentana;
            recvfrom(sockfd, (char *) &tamanoPaquete, sizeof(int), 0, (struct sockaddr *) &clienteAddr, &clienteAddrLen);
            recvfrom(sockfd, (char *) &tamanoVentana, sizeof(int), 0, (struct sockaddr *) &clienteAddr, &clienteAddrLen);
            printf("Cliente %d: Tamano del paquete: %d, Tamano de la ventana: %d\n", clienteExistente, tamanoPaquete, tamanoVentana);

            archivo = fopen("datos_recibidos.txt", "wb");
            if (archivo == NULL) {
                printf("Error al abrir el archivo");
                closesocket(sockfd);
                WSACleanup();
                return 1;
            }

            fwrite(buffer, 1, bytesRecibidos, archivo);
            fclose(archivo);      
        } else {
            // Retroceder-N
            retrocederNPaquetes(sockfd, &clienteAddr, &clienteAddrLen, buffer, &bytesRecibidos, archivo);
            continue; // Salta al siguiente ciclo del bucle while
        }    
    }

    closesocket(sockfd); // Cerrar socket
    WSACleanup(); // Limpiar Winsock

    return 0;
}

// Funciones

void retrocederNPaquetes(int sockfd, struct sockaddr_in * clienteAddr, int * clienteAddrLen, char * buffer, int * bytesRecibidos, FILE * archivo){
    int N = 3; // Número de paquetes a retroceder
    int retrocedidos = 0; // Número de paquetes retrocedidos

    while (retrocedidos < N){
        // Retroceder un paquete: leer el archivo y retransmitir
        fseek(archivo, -(*bytesRecibidos), SEEK_CUR);
        int bytesLeidos = fread(buffer, 1, TAMANO_PAQUETE, archivo);

        // Retransmitir el paquete
        sendto(sockfd, buffer, bytesLeidos, 0, (struct sockaddr *)clienteAddr, *clienteAddrLen);

        retrocedidos++;
    }
    printf("Se retrocedieron %d paquetes\n", retrocedidos);
}
