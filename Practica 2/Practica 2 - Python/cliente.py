import socket
import struct
import threading
import time

class SlidingWindowClient:
    def __init__(self, host, port, window_size, packet_size, filename):
        self.host = host
        self.port = port
        self.window_size = window_size
        self.packet_size = packet_size
        self.filename = filename
        self.window = [None] * window_size
        self.acks = [False] * window_size
        self.next_seq = 0
        self.base = 0
        self.lock = threading.Lock()
        self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.server_address = (host, port)
        self.eot = False

    def start(self):
        recv_thread = threading.Thread(target=self.receive_acks)
        recv_thread.start()
        self.send_file()
        recv_thread.join()

    def send_file(self):
        with open(self.filename, 'rb') as f:
            filename_data = struct.pack('!I', len(self.filename)) + self.filename.encode()
            initial_packet = self.create_packet(self.next_seq, False, filename_data)
            self.client_socket.sendto(initial_packet, self.server_address)
            self.next_seq += 1

            # Envía el contenido del archivo en paquetes
            while not self.eot or any(self.window):
                self.lock.acquire()
                while self.next_seq < self.base + self.window_size and not self.eot:
                    data = f.read(self.packet_size)
                    if not data:
                        self.eot = True
                        break
                    packet = self.create_packet(self.next_seq, self.eot, data)
                    self.window[self.next_seq % self.window_size] = packet
                    self.client_socket.sendto(packet, self.server_address)
                    self.next_seq += 1
                self.lock.release()
                time.sleep(0.1)
            self.client_socket.close()
            print("File sent successfully.")

    def receive_acks(self):
        while not self.eot or any(self.window):
            try:
                ack_packet, _ = self.client_socket.recvfrom(1024)
                seq_num, error = struct.unpack('!I?', ack_packet)
                if not error:
                    self.lock.acquire()
                    if seq_num >= self.base:
                        self.acks[seq_num % self.window_size] = True
                        while self.acks[self.base % self.window_size]:
                            self.acks[self.base % self.window_size] = False
                            self.window[self.base % self.window_size] = None
                            self.base += 1
                    self.lock.release()
            except:
                pass

    def create_packet(self, seq_num, eot, data):
        checksum = self.calculate_checksum(data)
        packet = struct.pack('!I?B', seq_num, eot, checksum) + data
        return packet
    
    def calculate_checksum(self, data):
        return sum(data) % 256
    
if __name__ == "__main__":
    host = 'localhost'
    port = 5000
    window_size = int(input("Tamanio de la ventana: "))
    packet_size = int(input("Tamanio del paquete: "))
    filename = input("Nombre del archivo a mandar: ")
    client = SlidingWindowClient(host, port, window_size, packet_size, filename)
    client.start()

