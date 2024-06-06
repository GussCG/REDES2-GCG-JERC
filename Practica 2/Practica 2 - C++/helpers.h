/*
    Archivo: helpers.h
    Practica 2. Implementacion de Ventanas Deslizantes con Retroceso N
    Autores:
        - Cerda Garcia Gustavo
        - Ramirez Carrillo Jose Emilio
*/

// Inclusion de librerias
#ifndef HELPERS_H
#define HELPERS_H

#include <chrono> // Biblioteca para el manejo de tiempo
#include <stdlib.h> 
#include <string.h>
#include <sys/types.h> // Biblioteca para el manejo de tipos de datos
#include <netinet/in.h> // Biblioteca para el manejo de direcciones de internet

#define MAX_DATA_SIZE 1024 // Tamaño maximo de los datos
#define MAX_FRAME_SIZE 1034 // Tamaño maximo del frame
#define ACK_SIZE 6 // Tamaño del ack

// Definicion de macros
#define current_time chrono::high_resolution_clock::now // Macro para obtener el tiempo actual
#define time_stamp chrono::high_resolution_clock::time_point // Macro para definir un punto en el tiempo
#define elapsed_time(end, start) chrono::duration_cast<chrono::milliseconds>(end - start).count() // Macro para obtener el tiempo transcurrido entre dos puntos en el tiempo
#define sleep_for(x) this_thread::sleep_for(chrono::milliseconds(x)); // Macro para dormir el hilo

typedef unsigned char byte; // Definicion de un byte

/*
    Nombre: checksum
    Desc: Funcion que calcula el checksum de un frame
    Args:
        - char *frame: Frame del cual se calculara el checksum
        - int count: Tamaño del frame
    Return:
        - char: Checksum del frame
*/
char checksum(char *frame, int count); 

/*
    Nombre: create_frame
    Desc: Funcion que crea un frame
    Args:
        - int seq_num: Numero de secuencia del frame
        - char *frame: Frame a crear
        - char *data: Datos del frame
        - int data_size: Tamaño de los datos
        - bool eot: Bandera de fin de transmision
    Return:
        - int: Tamaño del frame
*/
int create_frame(int seq_num, char *frame, char *data, int data_size, bool eot);

/*
    Nombre: create_ack
    Desc: Funcion que crea un ack
    Args:
        - int seq_num: Numero de secuencia del ack
        - char *ack: Ack a crear
        - bool error: Bandera de error
*/
void create_ack(int seq_num, char *ack, bool error);

/*
    Nombre: read_frame
    Desc: Funcion que lee un frame
    Args:
        - int *seq_num: Numero de secuencia del frame
        - char *data: Datos del frame
        - int *data_size: Tamaño de los datos
        - bool *eot: Bandera de fin de transmision
        - char *frame: Frame a leer
    Return:
        - bool: Si el checksum es correcto
*/
bool read_frame(int *seq_num, char *data, int *data_size, bool *eot, char *frame);

/*
    Nombre: read_ack
    Desc: Funcion que lee un ack
    Args:
        - int *seq_num: Numero de secuencia del ack
        - bool *neg: Bandera de error
        - char *ack: Ack a leer
    Return:
        - bool: Si el checksum es correcto
*/
bool read_ack(int *seq_num, bool *error, char *ack);

#endif