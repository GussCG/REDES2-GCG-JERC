/*
    Archivo: helpers.cpp
    Practica 2. Implementacion de Ventanas Deslizantes con Retroceso N
    Autores:
        - Cerda Garcia Gustavo
        - Ramirez Carrillo Jose Emilio
*/

// Inclusion de librerias propias
#include "helpers.h"

// Bibliotecas
#include <iostream>
using namespace std;

// Funciones
char checksum(char *frame, int count) {
    u_long sum = 0; // 32 bits
    // Mientras count sea diferente de 0 se suman los bytes del frame
    while (count--) {
        sum += *frame++; // Se suma el byte
        // Si el bit 16 esta activo se limpia y se suma 1
        if (sum & 0xFFFF0000) {
            sum &= 0xFFFF; // Se limpia el bit 16
            sum++; // Se suma 1
        }
    }
    return (sum & 0xFFFF); // Se regresa el checksum
}

int create_frame(int seq_num, char *frame, char *data, int data_size, bool eot) {
    frame[0] = eot ? 0x0 : 0x1; // Se asigna el bit de fin de transmision
    uint32_t net_seq_num = htonl(seq_num); // Se convierte el numero de secuencia a red
    uint32_t net_data_size = htonl(data_size); // Se convierte el tamaño de los datos a red
    memcpy(frame + 1, &net_seq_num, 4); // Se copia el numero de secuencia
    memcpy(frame + 5, &net_data_size, 4); // Se copia el tamaño de los datos
    memcpy(frame + 9, data, data_size); // Se copian los datos
    frame[data_size + 9] = checksum(frame, data_size + (int) 9); // Se asigna el checksum

    return data_size + (int)10; // Se regresa el tamaño del frame
}

void create_ack(int seq_num, char *ack, bool error) {
    ack[0] = error ? 0x0 : 0x1; // Se asigna el bit de error
    uint32_t net_seq_num = htonl(seq_num); // Se convierte el numero de secuencia a red
    memcpy(ack + 1, &net_seq_num, 4); // Se copia el numero de secuencia
    ack[5] = checksum(ack, ACK_SIZE - (int) 1); // Se asigna el checksum
}

bool read_frame(int *seq_num, char *data, int *data_size, bool *eot, char *frame) {
    *eot = frame[0] == 0x0 ? true : false; // Se asigna el bit de fin de transmision

    uint32_t net_seq_num; // Se declara el numero de secuencia en red
    memcpy(&net_seq_num, frame + 1, 4); // Se copia el numero de secuencia
    *seq_num = ntohl(net_seq_num); // Se convierte el numero de secuencia a host

    uint32_t net_data_size; // Se declara el tamaño de los datos en red
    memcpy(&net_data_size, frame + 5, 4); // Se copia el tamaño de los datos
    *data_size = ntohl(net_data_size); // Se convierte el tamaño de los datos a host

    memcpy(data, frame + 9, *data_size); // Se copian los datos 

    return frame[*data_size + 9] != checksum(frame, *data_size + (int) 9); // Se regresa si el checksum es correcto
}

bool read_ack(int *seq_num, bool *neg, char *ack) {
    *neg = ack[0] == 0x0 ? true : false; // Se asigna el bit de error

    uint32_t net_seq_num; // Se declara el numero de secuencia en red
    memcpy(&net_seq_num, ack + 1, 4); // Se copia el numero de secuencia
    *seq_num = ntohl(net_seq_num); // Se convierte el numero de secuencia a host

    return ack[5] != checksum(ack, ACK_SIZE - (int) 1); // Se regresa si el checksum es correcto
}