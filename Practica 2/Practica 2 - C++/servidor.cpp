/*
    Archivo: servidor.cpp
    Practica 2. Implementacion de Ventanas Deslizantes con Retroceso N
    Autores:
        - Cerda Garcia Gustavo
        - Ramirez Carrillo Jose Emilio
*/

// Inclusion de librerias
#include <iostream> // Biblioteca para la entrada y salida de datos
#include <thread> // Biblioteca para el manejo de hilos

#include <stdio.h> // Biblioteca para la entrada y salida estandar
#include <sys/socket.h> // Biblioteca para el manejo de sockets
#include <netdb.h> // Biblioteca para el manejo de direcciones de internet

// Inclusion de librerias propias
#include "helpers.h" 

// Definicion de macros
#define STDBY_TIME 3000

using namespace std;

int socket_fd; // Descriptor del socket
struct sockaddr_in server_addr, client_addr; // Direcciones del servidor y cliente

// Nombre: send_ack
// Desc: Funcion que envia un ack al cliente, un ack es un mensaje que confirma la recepcion de un frame
// Args: Ninguno
// Return: Ninguno
void send_ack() {
    char frame[MAX_FRAME_SIZE]; // Frame a recibir
    char data[MAX_DATA_SIZE]; // Datos del frame
    char ack[ACK_SIZE]; // Ack a enviar
    int frame_size; // Tamaño del frame
    int data_size; // Tamaño de los datos
    socklen_t client_addr_size; // Tamaño de la direccion del cliente
    
    int recv_seq_num; // Numero de secuencia del frame
    bool frame_error; // Bandera de error del frame
    bool eot; // Bandera de fin de transmision

    // Se recibe un frame y se envia un ack
    // Mientras no se reciba un frame se sigue en el ciclo
    while (true) {
        frame_size = recvfrom(socket_fd, (char *)frame, MAX_FRAME_SIZE, 
                MSG_WAITALL, (struct sockaddr *) &client_addr, 
                &client_addr_size); // Se recibe un frame
        frame_error = read_frame(&recv_seq_num, data, &data_size, &eot, frame); // Se lee el frame

        create_ack(recv_seq_num, ack, frame_error); // Se crea un ack
        sendto(socket_fd, ack, ACK_SIZE, 0,  
                (const struct sockaddr *) &client_addr, client_addr_size); // Se envia el ack
    }
}

int main(int argc, char * argv[]) {
    int port; // Puerto del servidor
    int window_len; // Tamaño de la ventana
    int max_buffer_size; // Tamaño maximo del buffer
    char *fname; // Nombre del archivo

    // Se verifica que se hayan pasado los argumentos correctos
    if (argc == 5) {
        fname = argv[1]; // Se obtiene el nombre del archivo
        window_len = (int) atoi(argv[2]); // Se obtiene el tamaño de la ventana
        max_buffer_size = MAX_DATA_SIZE * (int) atoi(argv[3]); // Se obtiene el tamaño maximo del buffer
        port = atoi(argv[4]); // Se obtiene el puerto
    } else {
        cerr << "usage: ./recvfile <filename> <window_size> <buffer_size> <port>" << endl; // Se imprime el uso correcto
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Se limpia la direccion del servidor
    memset(&client_addr, 0, sizeof(client_addr)); // Se limpia la direccion del cliente 
      
    // Se asignan los valores de la direccion del servidor
    server_addr.sin_family = AF_INET; // Se asigna el tipo de direccion
    server_addr.sin_addr.s_addr = INADDR_ANY; // Se asigna la direccion del servidor
    server_addr.sin_port = htons(port); // Se asigna el puerto

    // Se crea el socket
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cerr << "socket creation failed" << endl;
        return 1;
    }

    // Se asigna el socket a la direccion del servidor
    if (::bind(socket_fd, (const struct sockaddr *)&server_addr, 
            sizeof(server_addr)) < 0) { 
        cerr << "socket binding failed" << endl;
        return 1;
    }

    FILE *file = fopen(fname, "wb"); // Se abre el archivo para escritura en binario
    char buffer[max_buffer_size]; // Buffer para almacenar los datos
    int buffer_size; // Tamaño del buffer

    // Variables para el manejo de los frames
    char frame[MAX_FRAME_SIZE]; // Frame a recibir
    char data[MAX_DATA_SIZE]; // Datos del frame
    char ack[ACK_SIZE]; // Ack a enviar
    int frame_size; // Tamaño del frame
    int data_size; // Tamaño de los datos
    int lfr, laf; // Ultimo frame recibido y ultimo frame aceptado
    int recv_seq_num; // Numero de secuencia del frame
    bool eot; // Bandera de fin de transmision
    bool frame_error; // Bandera de error del frame

    // Se recibe el archivo
    bool recv_done = false; // Bandera de fin de recepcion
    int buffer_num = 0; // Numero de buffer

    // Mientras no se haya terminado de recibir el archivo se sigue en el ciclo
    while (!recv_done) {
        buffer_size = max_buffer_size; // Se asigna el tamaño del buffer
        memset(buffer, 0, buffer_size); // Se limpia el buffer
    
        int recv_seq_count = (int) max_buffer_size / MAX_DATA_SIZE; // Numero de secuencia del frame
        bool window_recv_mask[window_len]; // Mascara de recepcion de la ventana
        // Se inicializa la mascara de recepcion de la ventana
        for (int i = 0; i < window_len; i++) {
            window_recv_mask[i] = false; // Se asigna falso, no se ha recibido
        }
        lfr = -1; // Se asigna -1 al ultimo frame recibido
        laf = lfr + window_len; // Se asigna el ultimo frame aceptado
        
        // Se recibe un frame y se envia un ack
        // Mientras no se reciba un frame se sigue en el ciclo
        while (true) {
            socklen_t client_addr_size; // Tamaño de la direccion del cliente
            frame_size = recvfrom(socket_fd, (char *) frame, MAX_FRAME_SIZE, 
                    MSG_WAITALL, (struct sockaddr *) &client_addr, 
                    &client_addr_size); // Se recibe un frame
            frame_error = read_frame(&recv_seq_num, data, &data_size, &eot, frame); // Se lee el frame

            create_ack(recv_seq_num, ack, frame_error); // Se crea un ack
            sendto(socket_fd, ack, ACK_SIZE, 0, 
                    (const struct sockaddr *) &client_addr, client_addr_size); // Se envia el ack

            // Si el numero de secuencia es menor o igual al ultimo frame aceptado
            if (recv_seq_num <= laf) {
                // Se envia un ack con el numero de secuencia
                if (!frame_error) {
                    int buffer_shift = recv_seq_num * MAX_DATA_SIZE; // Se asigna el corrimiento del buffer

                    // Si el numero de secuencia es igual al ultimo frame recibido mas 1, se copian los datos
                    if (recv_seq_num == lfr + 1) {
                        memcpy(buffer + buffer_shift, data, data_size); // Se copian los datos

                        int shift = 1; // Corrimiento
                        // Se calcula el corrimiento
                        for (int i = 1; i < window_len; i++) {
                            if (!window_recv_mask[i]) break; // Si no se ha recibido se sale
                            shift += 1; // Se suma 1 al corrimiento, ya que se ha recibido
                        }
                        // Se realiza el corrimiento
                        for (int i = 0; i < window_len - shift; i++) {
                            window_recv_mask[i] = window_recv_mask[i + shift]; // Se asigna el valor de la mascara
                        }
                        // Se limpia la mascara
                        for (int i = window_len - shift; i < window_len; i++) {
                            window_recv_mask[i] = false; // Se asigna falso, no se ha recibido
                        }
                        lfr += shift; // Se asigna el corrimiento al ultimo frame recibido
                        laf = lfr + window_len; // Se asigna el ultimo frame aceptado
                    } else if (recv_seq_num > lfr + 1) {
                        // Si el numero de secuencia es mayor al ultimo frame recibido mas 1, se copian los datos
                        if (!window_recv_mask[recv_seq_num - (lfr + 1)]) {
                            memcpy(buffer + buffer_shift, data, data_size); // Se copian los datos
                            window_recv_mask[recv_seq_num - (lfr + 1)] = true; // Se asigna verdadero, se ha recibido
                        }
                    }

                    // Si se ha recibido el fin de transmision
                    if (eot) {
                        buffer_size = buffer_shift + data_size; // Se asigna el tamaño del buffer
                        recv_seq_count = recv_seq_num + 1; // Se asigna el numero de secuencia
                        recv_done = true; // Se asigna verdadero, se ha terminado de recibir
                    }
                }
            }
            
            // Si se ha recibido el ultimo frame se sale
            if (lfr >= recv_seq_count - 1) break;
        }

        cout << "\r" << "[RECIBIDO " << (unsigned long long) buffer_num * (unsigned long long) 
                max_buffer_size + (unsigned long long) buffer_size << " BYTES]" << flush; // Se imprime el numero de bytes recibidos
        fwrite(buffer, 1, buffer_size, file); // Se escribe el buffer en el archivo
        buffer_num += 1; // Se suma 1 al numero de buffer
    }

    fclose(file); // Se cierra el archivo

    // Se crea un hilo para enviar un ack
    thread stdby_thread(send_ack); // Se crea un hilo para enviar un ack
    time_stamp start_time = current_time(); // Se obtiene el tiempo actual
    // Se espera 3 segundos para enviar un ack
    while (elapsed_time(current_time(), start_time) < STDBY_TIME) {
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS | ]" << flush;
        sleep_for(100);
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS / ]" << flush;
        sleep_for(100);
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS - ]" << flush;
        sleep_for(100);
        cout << "\r" << "[STANDBY TO SEND ACK FOR 3 SECONDS \\ ]" << flush;
        sleep_for(100);
    }
    stdby_thread.detach(); // Se desvincula el hilo

    cout << "\nAcabo :)" << endl;
    return 0;
}