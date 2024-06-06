import os
import socket
import struct
import threading
from collections import defaultdict

class SlidingWindowServer:
    def __init__(self, host, port, window_size, packet_size):
        self.host = host
        self.port = port
        self.window_size = window_size
        self.packet_size = packet_size
        self.lock = threading.Lock()
        self.server_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server_socket.bind((host, port))
        self.connections = defaultdict(lambda: {"buffer": [], "expected_seq": 0})
        self.received_files_folder = "archivos_recibidos"
        if not os.path.exists(self.received_files_folder):
            os.makedirs(self.received_files_folder)

    def start(self):
        print(f"Server started at {self.host}:{self.port}")
        while True:
            data, addr = self.server_socket.recvfrom(self.packet_size + 8)
            threading.Thread(target=self.handle_packet, args=(data, addr)).start()

    def handle_packet(self, data, addr):
        seq_num, eot = struct.unpack('!I?', data[:5])
        checksum = data[5]
        packet_data = data[6:]

        if self.calculate_checksum(packet_data) != checksum:
            self.send_ack(addr, seq_num, error=True)
            return

        self.lock.acquire()
        connection = self.connections[addr]

        if seq_num == connection["expected_seq"]:
            connection["buffer"].append(packet_data)
            while connection["buffer"] and connection["buffer"][0] is not None:
                packet_data = connection["buffer"].pop(0)
                file_path = self.create_or_get_file(addr, packet_data)
                with open(file_path, 'ab') as file_handle:
                    if seq_num == 0:  # Si es el primer paquete, omitir toda esa información
                        file_handle.write(packet_data[4 + struct.unpack('!I', packet_data[:4])[0]:])
                    else:
                        file_handle.write(packet_data)
                    file_handle.flush()
                    connection["expected_seq"] += 1
            self.send_ack(addr, seq_num, error=False)
            if eot:
                print(f"Archivo recibido de {addr} guardado en {file_path}")
                self.send_ack(addr, seq_num, error=False)  # Confirmación adicional al cliente
        elif connection["expected_seq"] <= seq_num < connection["expected_seq"] + self.window_size:
            if connection["buffer"]:
                connection["buffer"][seq_num - connection["expected_seq"]] = packet_data
        self.lock.release()


    def create_or_get_file(self, addr, packet_data):
        if addr not in self.connections:
            filename_size = struct.unpack('!I', packet_data[:4])[0]
            filename = packet_data[4:4 + filename_size].decode().strip()
            file_path = os.path.join(self.received_files_folder, filename)
            if os.path.exists(file_path):
                os.remove(file_path)
            self.connections[addr] = {
                "file_path": file_path,
                "buffer": [],
                "expected_seq": 0
            }
        elif "file_path" not in self.connections[addr]:
            filename_size = struct.unpack('!I', packet_data[:4])[0]
            filename = packet_data[4:4 + filename_size].decode().strip()
            file_path = os.path.join(self.received_files_folder, filename)
            if os.path.exists(file_path):
                os.remove(file_path)
            self.connections[addr]["file_path"] = file_path
        return self.connections[addr]["file_path"]


    def send_ack(self, addr, seq_num, error):
        ack_packet = struct.pack('!I?', seq_num, error)
        self.server_socket.sendto(ack_packet, addr)

    @staticmethod
    def calculate_checksum(data):
        return sum(data) % 256

if __name__ == "__main__":
    host = '0.0.0.0'
    port = 5000
    window_size = int(input("Tamanio de la ventana: "))
    packet_size = int(input("Tamanio del paquete: "))

    server = SlidingWindowServer(host, port, window_size, packet_size)
    server.start()
