
package src;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.concurrent.TimeUnit;

public class Helpers {

    public static final int MAX_DATA_SIZE = 1024;
    public static final int MAX_FRAME_SIZE = 1034;
    public static final int ACK_SIZE = 6;

    public static byte checksum(byte[] frame, int count) {
        int sum = 0;
        for (int i = 0; i < count; i++) {
            sum += frame[i];
            if ((sum & 0xFFFF0000) != 0) {
                sum &= 0xFFFF;
                sum++;
            }
        }
        return (byte) (sum & 0xFFFF);
    }

    public static int createFrame(int seqNum, byte[] frame, byte[] data, int dataSize, boolean eot) {
        frame[0] = (byte) (eot ? 0x0 : 0x1);
        ByteBuffer.wrap(frame, 1, 4).order(ByteOrder.BIG_ENDIAN).putInt(seqNum);
        ByteBuffer.wrap(frame, 5, 4).order(ByteOrder.BIG_ENDIAN).putInt(dataSize);
        System.arraycopy(data, 0, frame, 9, dataSize);
        frame[dataSize + 9] = checksum(frame, dataSize + 9);
        return dataSize + 10;
    }

    public static void createAck(int seqNum, byte[] ack, boolean error) {
        ack[0] = (byte) (error ? 0x0 : 0x1);
        ByteBuffer.wrap(ack, 1, 4).order(ByteOrder.BIG_ENDIAN).putInt(seqNum);
        ack[5] = checksum(ack, ACK_SIZE - 1);
    }

    public static boolean readFrame(int[] seqNum, byte[] data, int[] dataSize, boolean[] eot, byte[] frame) {
        eot[0] = frame[0] == 0x0;
        seqNum[0] = ByteBuffer.wrap(frame, 1, 4).order(ByteOrder.BIG_ENDIAN).getInt();
        dataSize[0] = ByteBuffer.wrap(frame, 5, 4).order(ByteOrder.BIG_ENDIAN).getInt();
        System.arraycopy(frame, 9, data, 0, dataSize[0]);
        return frame[dataSize[0] + 9] != checksum(frame, dataSize[0] + 9);
    }

    public static boolean readAck(int[] seqNum, boolean[] error, byte[] ack) {
        error[0] = ack[0] == 0x0;
        seqNum[0] = ByteBuffer.wrap(ack, 1, 4).order(ByteOrder.BIG_ENDIAN).getInt();
        return ack[5] != checksum(ack, ACK_SIZE - 1);
    }

    public static long current_time() {
        return System.nanoTime();
    }

    public static long elapsed_time(long end, long start) {
        return TimeUnit.NANOSECONDS.toMillis(end - start);
    }

    public static void sleep_for(int x) {
        try {
            Thread.sleep(x);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
    }
}
