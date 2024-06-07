# Practica 2 - Ventana Deslizante y Regresar N en Python (CLIENTE)
# Autores:
#     Cerda García Gustavo
#     Ramirez Carrillo Jose Emilio
# Descripción:
#     Implementación de un cliente que envía archivos mediante el protocolo de ventana deslizante
#     y regresar N en Python.

# Librerías
import socket # Para manejar conexiones
import struct # Para manejar estructuras de datos binarias
import threading # Para manejar hilos 
import time # Para manejar tiempo

# Clase Cliente de Ventana Deslizante
# Descripción:
#     Clase que representa un cliente que envía archivos mediante el protocolo de ventana deslizante
#     y regresar N.
class SlidingWindowClient:
    # Método constructor
    # Parámetros:
    #     host (str): Dirección IP del servidor
    #     port (int): Puerto del servidor
    #     window_size (int): Tamaño de la ventana
    #     packet_size (int): Tamaño del paquete
    #     filename (str): Nombre del archivo a enviar
    def __init__(self, host, port, window_size, packet_size, filename):
        self.host = host
        self.port = port
        self.window_size = window_size
        self.packet_size = packet_size
        self.filename = filename
        self.window = [None] * window_size # Ventana de envío
        self.acks = [False] * window_size # Lista de ACKs
        self.next_seq = 0 # Siguiente número de secuencia
        self.base = 0 # Base de la ventana
        self.lock = threading.Lock() # Para manejar concurrencia
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # Crear socket de datagrama
        self.server_address = (host, port) # Dirección del servidor
        self.eot = False # Bandera de fin de transmisión

    # Método para iniciar el cliente
    # Descripción:
    #     Método que inicia el cliente y envía el archivo.
    def start(self):
        recv_thread = threading.Thread(target=self.receive_acks) # Crear hilo para recibir ACKs
        recv_thread.start() # Iniciar hilo para recibir ACKs
        self.send_file() # Enviar archivo
        recv_thread.join() # Esperar a que termine el hilo para recibir ACKs

    # Método para enviar archivo
    # Descripción:
    #     Método que envía el archivo al servidor.
    def send_file(self):
        # Enviar nombre del archivo
        with open(self.filename, 'rb') as f:
            filename_data = struct.pack('!I', len(self.filename)) + self.filename.encode() # Empaquetar nombre del archivo
            initial_packet = self.create_packet(self.next_seq, False, filename_data) # Crear paquete inicial
            self.client_socket.sendto(initial_packet, self.server_address) # Enviar paquete inicial
            self.next_seq += 1 # Actualizar número de secuencia

            # Envía el contenido del archivo en paquetes
            while not self.eot or any(self.window):
                self.lock.acquire() # Bloquear sección crítica
                # Si hay espacio en la ventana y no se ha llegado al final del archivo
                while self.next_seq < self.base + self.window_size and not self.eot:
                    data = f.read(self.packet_size) # Leer datos del archivo
                    # Si no hay datos, marcar fin de transmisión
                    if not data:
                        self.eot = True
                        break
                    packet = self.create_packet(self.next_seq, self.eot, data) # Crear paquete
                    self.window[self.next_seq % self.window_size] = packet # Guardar paquete en la ventana
                    self.client_socket.sendto(packet, self.server_address) # Enviar paquete
                    self.next_seq += 1 # Actualizar número de secuencia
                self.lock.release() # Desbloquear sección crítica
                time.sleep(0.1) # Esperar un tiempo
            self.client_socket.close() # Cerrar socket
            print("Archivo enviado exitosamente")

    # Método para recibir ACKs
    # Descripción:
    #     Método que recibe ACKs del servidor.
    def receive_acks(self):
        # Mientras no se haya llegado al final de la transmisión o haya paquetes en la ventana
        while not self.eot or any(self.window):
            try:
                ack_packet, _ = self.client_socket.recvfrom(1024) # Recibir paquete
                seq_num, error = struct.unpack('!I?', ack_packet) # Obtener número de secuencia y bandera de error
                if not error:
                    self.lock.acquire() # Bloquear sección crítica
                    # Si el número de secuencia es mayor o igual a la base
                    if seq_num >= self.base:
                        self.acks[seq_num % self.window_size] = True # Marcar ACK como recibido
                        # Mientras haya ACKs en la ventana
                        while self.acks[self.base % self.window_size]: 
                            self.acks[self.base % self.window_size] = False # Marcar ACK como no recibido
                            self.window[self.base % self.window_size] = None # Eliminar paquete de la ventana
                            self.base += 1 # Actualizar base
                    self.lock.release() # Desbloquear sección crítica
            except:
                pass # Ignorar errores
    
    # Método para crear paquete
    # Parámetros:
    #     seq_num (int): Número de secuencia
    #     eot (bool): Bandera de fin de transmisión
    #     data (bytes): Datos del paquete
    def create_packet(self, seq_num, eot, data):
        checksum = self.calculate_checksum(data) # Calcular checksum
        packet = struct.pack('!I?B', seq_num, eot, checksum) + data # Empaquetar datos
        return packet # Regresar paquete
    
    # Método para calcular checksum
    # Parámetros:
    #     data (bytes): Datos del paquete
    def calculate_checksum(self, data):
        return sum(data) % 256 # Regresar suma de los datos módulo 256
    
if __name__ == "__main__":
    host = 'localhost'
    port = 5000
    window_size = int(input("Tamanio de la ventana: "))
    packet_size = int(input("Tamanio del paquete: "))
    filename = input("Nombre del archivo a mandar: ")
    client = SlidingWindowClient(host, port, window_size, packet_size, filename) # Crear cliente
    client.start() # Iniciar cliente

