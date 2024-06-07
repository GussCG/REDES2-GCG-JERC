# Practica 2 - Ventana Deslizante y Regresar N en Python (SERVIDOR)
# Autores:
#     Cerda García Gustavo
#     Ramirez Carrillo Jose Emilio
# Descripción:
#     Implementación de un servidor que recibe archivos mediante el protocolo de ventana deslizante
#     y regresar N en Python.

# Librerías
import os # Para manejar archivos
import socket # Para manejar conexiones
import struct # Para manejar estructuras de datos binarias
import threading # Para manejar hilos
from collections import defaultdict # Para manejar diccionarios con valores por defecto

# Clase Servidor de Ventana Deslizante
# Descripción:
#     Clase que representa un servidor que recibe archivos mediante el protocolo de ventana deslizante
#     y regresar N.
class SlidingWindowServer:
    # Método constructor
    # Parámetros:
    #     host (str): Dirección IP del servidor
    #     port (int): Puerto del servidor
    #     window_size (int): Tamaño de la ventana
    #     packet_size (int): Tamaño del paquete
    def __init__(self, host, port, window_size, packet_size):
        self.host = host 
        self.port = port
        self.window_size = window_size
        self.packet_size = packet_size
        self.lock = threading.Lock() # Para manejar concurrencia
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM) # Crear socket de datagrama
        self.server_socket.bind((host, port)) # Enlazar socket a dirección y puerto
        self.connections = defaultdict(lambda: {"buffer": [], "expected_seq": 0}) # Conexiones con clientes
        self.received_files_folder = "archivos_recibidos" # Carpeta para guardar archivos recibidos
        # Crear carpeta para guardar archivos recibidos si no existe
        if not os.path.exists(self.received_files_folder):
            os.makedirs(self.received_files_folder)

    # Método para iniciar el servidor
    # Descripción:
    #     Método que inicia el servidor y espera conexiones de clientes.
    def start(self):
        print(f"Server started at {self.host}:{self.port}")
        print("Esperando conexión de un cliente...")
        while True:
            # Si se ha conectado un cliente imprime su dirección y crea un hilo para manejar la conexión
            data, addr = self.server_socket.recvfrom(self.packet_size + 8) # Recibir paquete
            threading.Thread(target=self.handle_packet, args=(data, addr)).start() # Crear hilo para manejar paquete
            print(f"Recibidos {len(data)} bytes de {addr[0]}")

    # Método para manejar paquetes
    # Parámetros:
    #     data (bytes): Datos del paquete
    #     addr (tuple): Dirección del cliente
    def handle_packet(self, data, addr):
        seq_num, eot = struct.unpack('!I?', data[:5]) # Obtener número de secuencia y bandera de fin de transmisión
        checksum = data[5] # Obtener checksum
        packet_data = data[6:] # Obtener datos del paquete

        # Verificar checksum
        # Si el checksum no coincide, enviar ACK con error
        if self.calculate_checksum(packet_data) != checksum:
            self.send_ack(addr, seq_num, error=True) # Enviar ACK con error
            return

        self.lock.acquire() # Bloquear acceso a variables compartidas
        connection = self.connections[addr] # Obtener conexión con el cliente
        print(f"Recibido paquete {seq_num} de {addr[0]}")

        # Si el número de secuencia es el esperado, guardar datos del paquete
        if seq_num == connection["expected_seq"]:
            connection["buffer"].append(packet_data) # Guardar datos del paquete
            # Mientras haya paquetes en el buffer y el siguiente paquete esperado no sea nulo
            while connection["buffer"] and connection["buffer"][0] is not None:
                packet_data = connection["buffer"].pop(0) # Obtener paquete del buffer
                file_path = self.create_or_get_file(addr, packet_data) # Crear o obtener archivo
                # Si es el último paquete, guardar datos y enviar ACK
                with open(file_path, 'ab') as file_handle:
                    if seq_num == 0:  # Si es el primer paquete, omitir toda esa información
                        file_handle.write(packet_data[4 + struct.unpack('!I', packet_data[:4])[0]:]) # Escribir datos
                    else:
                        file_handle.write(packet_data) # Escribir datos
                    file_handle.flush() # Limpiar buffer
                    connection["expected_seq"] += 1 # Actualizar número de secuencia esperado
            self.send_ack(addr, seq_num, error=False) # Enviar ACK
            if eot:
                print(f"Archivo recibido de {addr} guardado en {file_path}")
                self.send_ack(addr, seq_num, error=False)  # Confirmación adicional al cliente
        # Si el número de secuencia está dentro de la ventana, guardar datos del paquete
        elif connection["expected_seq"] <= seq_num < connection["expected_seq"] + self.window_size:
            # Si el buffer no tiene suficiente espacio, agregar paquetes nulos
            if connection["buffer"]:
                connection["buffer"][seq_num - connection["expected_seq"]] = packet_data
        self.lock.release() # Desbloquear acceso a variables compartidas

    # Método para crear o obtener archivo
    # Parámetros:
    #     addr (tuple): Dirección del cliente
    #     packet_data (bytes): Datos del paquete
    def create_or_get_file(self, addr, packet_data):
        # Si la dirección no está en las conexiones, crear archivo
        if addr not in self.connections:
            filename_size = struct.unpack('!I', packet_data[:4])[0] # Obtener tamaño del nombre del archivo
            filename = packet_data[4:4 + filename_size].decode().strip() # Obtener nombre del archivo
            file_path = os.path.join(self.received_files_folder, filename) # Obtener ruta del archivo
            # Si el archivo ya existe, eliminarlo
            if os.path.exists(file_path):
                os.remove(file_path) # Eliminar archivo
            # Guardar información de la conexión
            self.connections[addr] = {
                "file_path": file_path,
                "buffer": [],
                "expected_seq": 0
            }
        # Si la dirección está en las conexiones, obtener ruta del archivo
        elif "file_path" not in self.connections[addr]:
            filename_size = struct.unpack('!I', packet_data[:4])[0] # Obtener tamaño del nombre del archivo
            filename = packet_data[4:4 + filename_size].decode().strip() # Obtener nombre del archivo
            file_path = os.path.join(self.received_files_folder, filename) # Obtener ruta del archivo
            # Si el archivo ya existe, eliminarlo
            if os.path.exists(file_path):
                os.remove(file_path)
            self.connections[addr]["file_path"] = file_path # Guardar ruta del archivo
        return self.connections[addr]["file_path"] # Retornar ruta del archivo

    # Método para enviar ACK
    # Parámetros:
    #     addr (tuple): Dirección del cliente
    #     seq_num (int): Número de secuencia
    #     error (bool): Indica si hubo un error
    def send_ack(self, addr, seq_num, error):
        ack_packet = struct.pack('!I?', seq_num, error) # Empaquetar ACK
        self.server_socket.sendto(ack_packet, addr) # Enviar ACK

    @staticmethod
    # Método para calcular checksum
    def calculate_checksum(data):
        return sum(data) % 256 # Calcular checksum

if __name__ == "__main__":
    host = '0.0.0.0'
    port = 5000
    window_size = int(input("Tamanio de la ventana: "))
    packet_size = int(input("Tamanio del paquete: "))

    server = SlidingWindowServer(host, port, window_size, packet_size) # Crear servidor
    server.start()