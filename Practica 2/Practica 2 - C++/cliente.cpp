/*
    Archivo: cliente.cpp
    Practica 2. Implementacion de Ventanas Deslizantes con Retroceso N
    Autores:
        - Cerda Garcia Gustavo
        - Ramirez Carrillo Jose Emilio
*/

// Inclusion de librerias
#include <iostream> // Biblioteca para la entrada y salida de datos
#include <thread> // Biblioteca para el manejo de hilos
#include <mutex> // Biblioteca para el manejo de mutex, semaforos y candados

#include <stdio.h> // Biblioteca para la entrada y salida estandar
#include <unistd.h> // Biblioteca para el manejo de llamadas al sistema
#include <sys/socket.h> // Biblioteca para el manejo de sockets
#include <netdb.h> // Biblioteca para el manejo de direcciones de internet

// Inclusion de librerias propias
#include "helpers.h"

#define TIMEOUT 10 // Tiempo de espera para reenviar un frame

using namespace std;

int socket_fd; // Descriptor del socket
struct sockaddr_in server_addr, client_addr; // Direcciones del servidor y cliente

int window_len; // Tamaño de la ventana
bool *window_ack_mask; // Mascara de acks de la ventana
time_stamp *window_sent_time; // Tiempo de envio de los frames de la ventana
int lar, lfs; // Variables de la ventana

time_stamp TMIN = current_time(); // Tiempo minimo
mutex window_info_mutex; // Mutex para la informacion de la ventana

// Nombre: listen_ack
// Desc: Funcion que escucha los acks del servidor
// Args: Ninguno
// Return: Ninguno
void listen_ack() {
    char ack[ACK_SIZE]; // Ack a recibir
    int ack_size; // Tamaño del ack
    int ack_seq_num; // Numero de secuencia del ack
    bool ack_error; // Bandera de error del ack
    bool ack_neg; // Bandera de negacion del ack

    // Se recibe un ack y se actualiza la ventana
    // Mientras no se reciba un ack se sigue en el ciclo
    while (true) {
        socklen_t server_addr_size; // Tamaño de la direccion del servidor
        ack_size = recvfrom(socket_fd, (char *)ack, ACK_SIZE, 
                MSG_WAITALL, (struct sockaddr *) &server_addr, 
                &server_addr_size); // Se recibe un ack
        ack_error = read_ack(&ack_seq_num, &ack_neg, ack); // Se lee el ack

        window_info_mutex.lock(); // Se bloquea el mutex

        // Se actualiza la ventana
        if (!ack_error && ack_seq_num > lar && ack_seq_num <= lfs) {
            // Si el ack es positivo se marca como recibido
            if (!ack_neg) {
                window_ack_mask[ack_seq_num - (lar + 1)] = true; // Se marca el ack como recibido
            } else {
                window_sent_time[ack_seq_num - (lar + 1)] = TMIN; // Se actualiza el tiempo de envio
            }
        }

        window_info_mutex.unlock(); // Se desbloquea el mutex
    }
}

int main(int argc, char *argv[]) {
    char *dest_ip; // IP del servidor
    int dest_port; // Puerto del servidor
    int max_buffer_size; // Tamaño maximo del buffer
    struct hostent *dest_hnet; // Host del servidor
    char *fname; // Nombre del archivo

    // Se verifica que se hayan pasado los argumentos correctos
    if (argc == 6) {
        fname = argv[1]; // Se obtiene el nombre del archivo
        window_len = atoi(argv[2]); // Se obtiene el tamaño de la ventana
        max_buffer_size = MAX_DATA_SIZE * (int) atoi(argv[3]); // Se obtiene el tamaño maximo del buffer
        dest_ip = argv[4]; // Se obtiene la IP del servidor
        dest_port = atoi(argv[5]); // Se obtiene el puerto del servidor
    } else {
        cerr << "usage: ./sendfile <filename> <window_len> <buffer_size> <destination_ip> <destination_port>" << endl;
        return 1; 
    }

    // Se obtiene la direccion del servidor
    dest_hnet = gethostbyname(dest_ip);  
    // Si no se encuentra la direccion se imprime un error
    if (!dest_hnet) {
        cerr << "unknown host: " << dest_ip << endl;
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Se limpia la direccion del servidor
    memset(&client_addr, 0, sizeof(client_addr)); // Se limpia la direccion del cliente

    // Se asignan los valores de la direccion del servidor
    server_addr.sin_family = AF_INET; // Se asigna el tipo de direccion
    bcopy(dest_hnet->h_addr, (char *)&server_addr.sin_addr, 
            dest_hnet->h_length); // Se asigna la direccion del servidor
    server_addr.sin_port = htons(dest_port); // Se asigna el puerto

    // Se asignan los valores de la direccion del cliente
    client_addr.sin_family = AF_INET; // Se asigna el tipo de direccion
    client_addr.sin_addr.s_addr = INADDR_ANY; // Se asigna la direccion del cliente
    client_addr.sin_port = htons(0); // Se asigna el puerto

    // Se crea el socket
    if ((socket_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        cerr << "socket creation failed" << endl;
        return 1;
    }

    // Se asigna el socket a la direccion del cliente
    if (::bind(socket_fd, (const struct sockaddr *)&client_addr, 
            sizeof(client_addr)) < 0) { 
        cerr << "socket binding failed" << endl;
        return 1;
    }

    // Se verifica que el archivo exista
    if (access(fname, F_OK) == -1) {
        cerr << "file doesn't exist: " << fname << endl;
        return 1;
    }

    // Se abre el archivo
    FILE *file = fopen(fname, "rb"); // Se abre el archivo para lectura en binario
    char buffer[max_buffer_size]; // Buffer para almacenar los datos
    int buffer_size; // Tamaño del buffer

    // Se inicializa el hilo para escuchar los acks
    thread recv_thread(listen_ack);

    // Variables para el manejo de los frames
    char frame[MAX_FRAME_SIZE]; // Frame a enviar
    char data[MAX_DATA_SIZE]; // Datos del frame
    int frame_size; // Tamaño del frame
    int data_size; // Tamaño de los datos

    // Variables para el manejo de los buffers
    bool read_done = false; // Bandera de fin de lectura
    int buffer_num = 0; // Numero de buffer

    // Se envian los buffers
    // Mientras no se haya terminado de leer el archivo se sigue en el ciclo
    while (!read_done) {

        // Se lee un buffer
        buffer_size = fread(buffer, 1, max_buffer_size, file); // Se lee un buffer
        // Si el tamaño del buffer es igual al maximo se lee un byte mas
        if (buffer_size == max_buffer_size) {
            char temp[1]; // Buffer temporal
            int next_buffer_size = fread(temp, 1, 1, file); // Se lee un byte
            if (next_buffer_size == 0) read_done = true; // Si no se lee nada se termina
            int error = fseek(file, -1, SEEK_CUR); // Se regresa un byte
        } else if (buffer_size < max_buffer_size) {
            // Si el tamaño del buffer es menor al maximo se termina
            read_done = true;
        }
        
        window_info_mutex.lock(); // Se bloquea el mutex

        // Se inicializan las variables de la ventana
        int seq_count = buffer_size / MAX_DATA_SIZE + ((buffer_size % MAX_DATA_SIZE == 0) ? 0 : 1); // Numero de secuencia
        int seq_num; // Numero de secuencia
        window_sent_time = new time_stamp[window_len]; // Tiempo de envio de los frames de la ventana
        window_ack_mask = new bool[window_len]; // Mascara de acks de la ventana
        bool window_sent_mask[window_len]; // Mascara de envio de la ventana
        // Se inicializan las mascaras de la ventana
        for (int i = 0; i < window_len; i++) {
            window_ack_mask[i] = false; // Se asigna falso, no se ha recibido
            window_sent_mask[i] = false; // Se asigna falso, no se ha enviado
        }
        lar = -1; // Se asigna -1 al ultimo frame recibido
        lfs = lar + window_len; // Se asigna el ultimo frame aceptado

        window_info_mutex.unlock(); // Se desbloquea el mutex
        
        // Se envian los frames
        bool send_done = false; // Bandera de fin de envio
        // Mientras no se haya terminado de enviar los frames se sigue en el ciclo
        while (!send_done) {

            window_info_mutex.lock(); // Se bloquea el mutex

            // Se actualiza la ventana
            if (window_ack_mask[0]) {
                int shift = 1; // Corrimiento
                // Se busca el corrimiento
                for (int i = 1; i < window_len; i++) {
                    if (!window_ack_mask[i]) break; // Si no se ha recibido se termina
                    shift += 1; // Se suma 1 al corrimiento
                }
                // Se actualiza la ventana
                for (int i = 0; i < window_len - shift; i++) {
                    window_sent_mask[i] = window_sent_mask[i + shift]; // Se actualiza la mascara de envio
                    window_ack_mask[i] = window_ack_mask[i + shift]; // Se actualiza la mascara de acks
                    window_sent_time[i] = window_sent_time[i + shift]; // Se actualiza el tiempo de envio
                }
                // Se limpian las mascaras
                for (int i = window_len - shift; i < window_len; i++) {
                    window_sent_mask[i] = false; // Se asigna falso, no se ha enviado
                    window_ack_mask[i] = false; // Se asigna falso, no se ha recibido
                }
                lar += shift; // Se actualiza el ultimo frame recibido
                lfs = lar + window_len; // Se actualiza el ultimo frame aceptado
            }

            window_info_mutex.unlock(); // Se desbloquea el mutex

            // Se envian los frames de la ventana
            for (int i = 0; i < window_len; i ++) {
                seq_num = lar + i + 1; // Se asigna el numero de secuencia

                // Si el numero de secuencia es menor al numero de secuencia total se envia
                if (seq_num < seq_count) {
                    window_info_mutex.lock(); // Se bloquea el mutex

                    // Si no se ha enviado o no se ha recibido el ack se envia
                    if (!window_sent_mask[i] || (!window_ack_mask[i] && (elapsed_time(current_time(), window_sent_time[i]) > TIMEOUT))) {
                        int buffer_shift = seq_num * MAX_DATA_SIZE; // Corrimiento del buffer
                        data_size = (buffer_size - buffer_shift < MAX_DATA_SIZE) ? (buffer_size - buffer_shift) : MAX_DATA_SIZE; // Tamaño de los datos
                        memcpy(data, buffer + buffer_shift, data_size); // Se copian los datos
                        
                        bool eot = (seq_num == seq_count - 1) && (read_done); // Bandera de fin de transmision
                        frame_size = create_frame(seq_num, frame, data, data_size, eot); // Se crea el frame

                        sendto(socket_fd, frame, frame_size, 0, 
                                (const struct sockaddr *) &server_addr, sizeof(server_addr)); // Se envia el frame
                        window_sent_mask[i] = true; // Se marca el frame como enviado
                        window_sent_time[i] = current_time(); // Se actualiza el tiempo de envio
                    }

                    window_info_mutex.unlock(); // Se desbloquea el mutex
                }
            }

            // Si el ultimo frame aceptado es igual al ultimo frame se termina
            if (lar >= seq_count - 1) send_done = true;
        }

        // Se imprime el progreso
        cout << "\r" << "[ENVIADO " << (unsigned long long) buffer_num * (unsigned long long) 
                max_buffer_size + (unsigned long long) buffer_size << " BYTES]" << flush;
        buffer_num += 1;
        if (read_done) break; // Si se termino de leer se termina
    }
    
    fclose(file); // Se cierra el archivo
    delete [] window_ack_mask; // Se libera la memoria
    delete [] window_sent_time; // Se libera la memoria
    recv_thread.detach(); // Se libera el hilo

    cout << "\nAcabo :)" << endl;
    return 0;
}