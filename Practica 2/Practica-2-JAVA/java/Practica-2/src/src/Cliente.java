
package src;

import java.io.*;
import java.net.*;
import java.util.Arrays;

public class Cliente {

    static final int TIMEOUT = 10;

    static DatagramSocket socket;
    static InetAddress serverAddr, clientAddr;
    static int windowLen;
    static boolean[] windowAckMask;
    static long[] windowSentTime;
    static int lar, lfs;
    static long TMIN = System.currentTimeMillis();
    static Object windowInfoMutex = new Object();

    public static void listenAck() {
        byte[] ack = new byte[Helpers.ACK_SIZE];
        int ackSize;
        int ackSeqNum = 0;
        boolean ackError;
        boolean ackNeg = false;

        /* Listen for ack from receiver */
        while (true) {
            DatagramPacket packet = new DatagramPacket(ack, ack.length);
            try {
                socket.receive(packet);
                ackSize = packet.getLength();
                ackError = Helpers.readAck(new int[]{}, new boolean[]{}, ack);

                synchronized (windowInfoMutex) {
                    if (!ackError && ackSeqNum > lar && ackSeqNum <= lfs) {
                        if (!ackNeg) {
                            windowAckMask[ackSeqNum - (lar + 1)] = true;
                        } else {
                            windowSentTime[ackSeqNum - (lar + 1)] = TMIN;
                        }
                    }
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public static void main(String[] args) {
        String fname;
        int destPort;
        int maxBufferSize;
        InetAddress destAddr = null;

        if (args.length == 6) {
            fname = args[0];
            windowLen = Integer.parseInt(args[1]);
            maxBufferSize = Helpers.MAX_DATA_SIZE * Integer.parseInt(args[2]);
            destPort = Integer.parseInt(args[5]);
        } else {
            // System.err.println("usage: ./sendfile <filename> <window_len> <buffer_size> <destination_ip> <destination_port>");
            // return;
            fname = "test.txt";
            windowLen = 4;
            maxBufferSize = Helpers.MAX_DATA_SIZE * 4;

            destPort = 12345;
        }

        try {
            destAddr = InetAddress.getLoopbackAddress();
            socket = new DatagramSocket();
        } catch (SocketException e) {
            e.printStackTrace();
            return;
        }

        byte[] buffer = new byte[maxBufferSize];
        int bufferLength;

        try (FileInputStream fileInputStream = new FileInputStream(fname)) {

            Thread recvThread = new Thread(Cliente::listenAck);
            recvThread.start();

            byte[] frame = new byte[Helpers.MAX_FRAME_SIZE];
            byte[] data = new byte[Helpers.MAX_DATA_SIZE];
            int dataSize;

            while ((bufferLength = fileInputStream.read(buffer)) != -1) {
                synchronized (windowInfoMutex) {
                    int seqCount = bufferLength / Helpers.MAX_DATA_SIZE + ((bufferLength % Helpers.MAX_DATA_SIZE == 0) ? 0 : 1);
                    int seqNum;
                    windowSentTime = new long[windowLen];
                    windowAckMask = new boolean[windowLen];
                    boolean[] windowSentMask = new boolean[windowLen];
                    Arrays.fill(windowAckMask, false);
                    Arrays.fill(windowSentMask, false);
                    lar = -1;
                    lfs = lar + windowLen;

                    boolean sendDone = false;
                    while (!sendDone) {
                        if (windowAckMask[0]) {
                            int shift = 1;
                            for (int i = 1; i < windowLen; i++) {
                                if (!windowAckMask[i]) break;
                                shift++;
                            }
                            for (int i = 0; i < windowLen - shift; i++) {
                                windowSentMask[i] = windowSentMask[i + shift];
                                windowAckMask[i] = windowAckMask[i + shift];
                                windowSentTime[i] = windowSentTime[i + shift];
                            }
                            for (int i = windowLen - shift; i < windowLen; i++) {
                                windowSentMask[i] = false;
                                windowAckMask[i] = false;
                            }
                            lar += shift;
                            lfs = lar + windowLen;
                        }

                        for (int i = 0; i < windowLen; i++) {
                            seqNum = lar + i + 1;

                            if (seqNum < seqCount) {
                                if (!windowSentMask[i] || (!windowAckMask[i] && (System.currentTimeMillis() - windowSentTime[i] > TIMEOUT))) {
                                    int bufferShift = seqNum * Helpers.MAX_DATA_SIZE;
                                    dataSize = (bufferLength - bufferShift < Helpers.MAX_DATA_SIZE) ? (bufferLength - bufferShift) : Helpers.MAX_DATA_SIZE;
                                    System.arraycopy(buffer, bufferShift, data, 0, dataSize);

                                    boolean eot = (seqNum == seqCount - 1) && (bufferLength < maxBufferSize);
                                    int frameSize = Helpers.createFrame(seqNum, frame, data, dataSize, eot);

                                    DatagramPacket packet = new DatagramPacket(frame, frameSize, destAddr, destPort);
                                    try {
                                        socket.send(packet);
                                    } catch (IOException e) {
                                        e.printStackTrace();
                                    }
                                    windowSentMask[i] = true;
                                    windowSentTime[i] = System.currentTimeMillis();
                                }
                            }
                        }
                        if (lar >= seqCount - 1) sendDone = true;
                    }
                }
            }

            socket.close();
            recvThread.join();

        } catch (IOException | InterruptedException e) {
            e.printStackTrace();
        }

        System.out.println("\nAll done :)");
    }
}