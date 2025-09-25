package com.unicamp.ebpfagent;

import com.sun.jna.Pointer;

public class RingBufferReader {

    private int mapFd;
    private Pointer buffer;

    public RingBufferReader(int mapFd, int bufferSize) {
        this.mapFd = mapFd;
        this.buffer = LibC.INSTANCE.mmap(
            Pointer.NULL,
            bufferSize,
            LibC.PROT_READ | LibC.PROT_WRITE,
            LibC.MAP_SHARED,
            mapFd,
            0
        );

        if (Pointer.nativeValue(buffer) == -1) {
            throw new RuntimeException("mmap failed");
        }
    }

    public void pollAndRead() {
        PollFd pfd = new PollFd();
        pfd.fd = mapFd;
        pfd.events = (short) 1; // POLLIN

        int ret = LibC.INSTANCE.poll(new PollFd[]{pfd}, 1, 1000);
        if (ret > 0 && (pfd.revents & 1) != 0) {
            byte[] buf = new byte[256]; // depende do tamanho do evento
            int read = LibC.INSTANCE.read(mapFd, buf, buf.length);
            if (read > 0) {
                String msg = new String(buf, 0, read);
                System.out.println("Event: " + msg);
            }
        }
    }

    public void close(int bufferSize) {
        LibC.INSTANCE.munmap(buffer, bufferSize);
    }
}
