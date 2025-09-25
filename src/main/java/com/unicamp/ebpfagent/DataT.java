package com.unicamp.ebpfagent;

import com.sun.jna.Pointer;
import java.nio.charset.StandardCharsets;

public record DataT(int pid, int uid, String command, String message) {
    // sizes / offsets must match the C struct layout exactly
    // C struct:
    // int pid;           // 4 bytes (offset 0)
    // int uid;           // 4 bytes (offset 4)
    // char command[16];  // 16 bytes (offset 8..23)
    // char message[12];  // 12 bytes (offset 24..35)
    public static final int SIZE = 4 + 4 + 16 + 12; // 36

    public static DataT fromPointer(Pointer p, long size) {
        if (size < SIZE) {
            throw new IllegalArgumentException("Event size " + size + " < expected " + SIZE);
        }
        // read ints (little endian on x86_64)
        int pid = p.getInt(0);
        int uid = p.getInt(4);
        String comm = p.getString(8);
        String msg = p.getString(24);
        return new DataT(pid, uid, comm, msg);
    }

    @Override
    public String toString() {
        return "DataT{pid=" + pid + ", uid=" + uid + ", command='" + command + "', message='" + message + "'}";
    }
}
