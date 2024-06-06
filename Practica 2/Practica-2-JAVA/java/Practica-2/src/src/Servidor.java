
package src;

import java.io.*;
import java.net.*;
import java.util.Arrays;

public class Servidor {

    static int STDBY_TIME = 3000;
    static DatagramSocket socket;
    static InetAddress serverAddr, clientAddr;
    static int windowLen;
    static boolean[] windowRecvMask;
    static int lfr, laf;

    public static void sendAck(int recvSeqNum, boolean frameError, DatagramPacket packet) throws IOException {
        byte[] ack = new byte[Helpers.ACK_SIZE];
        Helpers.createAck(recvSeqNum, ack, frameError);
        DatagramPacket ackPacket = new DatagramPacket(ack, ack.length, packet.getAddress(), packet.getPort());
        socket.send(ackPacket);
    }

    public static void main(String[] args) {
        int port;
        int maxBufferSize;
        String fname;

        if (args.length == 4) {
            fname = args[0];
            windowLen = Integer.parseInt(args[1]);
            maxBufferSize = Helpers.MAX_DATA_SIZE * Integer.parseInt(args[2]);
            port = Integer.parseInt(args[3]);
        } else {
            // System.err.println("usage: ./recvfile <filename> <window_size> <buffer_size> <port>");
            // return;
            fname = "test.txt";
            windowLen = 4;
            maxBufferSize = Helpers.MAX_DATA_SIZE * 4;
            port = 12345;
        }

        try {
            socket = new DatagramSocket(port);
        } catch (SocketException e) {
            e.printStackTrace();
            return;
        }

        byte[] buffer = new byte[maxBufferSize];
        byte[] frame = new byte[Helpers.MAX_FRAME_SIZE];
        byte[] data = new byte[Helpers.MAX_DATA_SIZE];
        int dataSize = Helpers.MAX_DATA_SIZE;

        boolean eot;
        boolean frameError;
        int recvSeqNum = 0;

        try (FileOutputStream fileOutputStream = new FileOutputStream(fname)) {

            while (true) {
                int bufferSize = maxBufferSize;
                eot = false;
                Arrays.fill(buffer, (byte) 0);

                int recvSeqCount = maxBufferSize / Helpers.MAX_DATA_SIZE;
                windowRecvMask = new boolean[windowLen];
                Arrays.fill(windowRecvMask, false);
                lfr = -1;
                laf = lfr + windowLen;

                while (true) {
                    DatagramPacket packet = new DatagramPacket(frame, frame.length);
                    socket.receive(packet);
                    int frameSize = packet.getLength();
                    frameError = Helpers.readFrame(new int[1], data, new int[1], new boolean[1], frame);

                    sendAck(recvSeqNum, frameError, packet);

                    if (recvSeqNum <= laf) {
                        if (!frameError) {
                            int bufferShift = recvSeqNum * Helpers.MAX_DATA_SIZE;

                            if (recvSeqNum == lfr + 1) {
                                System.arraycopy(data, 0, buffer, bufferShift, dataSize);

                                int shift = 1;
                                for (int i = 1; i < windowLen; i++) {
                                    if (!windowRecvMask[i]) break;
                                    shift += 1;
                                }
                                for (int i = 0; i < windowLen - shift; i++) {
                                    windowRecvMask[i] = windowRecvMask[i + shift];
                                }
                                for (int i = windowLen - shift; i < windowLen; i++) {
                                    windowRecvMask[i] = false;
                                }
                                lfr += shift;
                                laf = lfr + windowLen;
                            } else if (recvSeqNum > lfr + 1) {
                                if (!windowRecvMask[recvSeqNum - (lfr + 1)]) {
                                    System.arraycopy(data, 0, buffer, bufferShift, dataSize);
                                    windowRecvMask[recvSeqNum - (lfr + 1)] = true;
                                }
                            }

                            if (eot) {
                                bufferSize = bufferShift + dataSize;
                                recvSeqCount = recvSeqNum + 1;
                                break;
                            }
                        }
                    }

                    if (lfr >= recvSeqCount - 1) break;
                }

                // fileOutputStream.write(buffer, 0, bufferSize);
            }
        } catch (IOException e) {
            e.printStackTrace();
        }

        socket.close();

        System.out.println("\nAll done :)");
    }
}