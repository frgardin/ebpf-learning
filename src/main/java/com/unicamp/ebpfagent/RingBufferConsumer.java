package com.unicamp.ebpfagent;

import com.sun.jna.Pointer;

public class RingBufferConsumer {

    private final Pointer rb;
    private final LibBpf.RingBufferSampleFn cb;

    public RingBufferConsumer(int mapFd) {
        // define callback: will be called from native libbpf thread (JNI -> be careful with heavy work)
        cb = new LibBpf.RingBufferSampleFn() {
            @Override
            public int invoke(Pointer ctx, Pointer data, long size) {
                try {
                    DataT ev = DataT.fromPointer(data, size);
                    // aqui processa: imprime, envia para Prometheus, etc.
                    System.out.println(ev);
                } catch (Exception e) {
                    System.err.println("Failed to parse event: " + e);
                }
                return 0; // return 0 indicates success
            }
        };

        // create ring buffer object
        rb = LibBpf.INSTANCE.ring_buffer__new(mapFd, cb, Pointer.NULL, Pointer.NULL);
        if (rb == null) {
            throw new RuntimeException("ring_buffer__new returned NULL (check map FD and libbpf availability)");
        }
    }

    /** Polls once (timeout ms). Returns poll result or throws on error. */
    public int poll(int timeoutMs) {
        int ret = LibBpf.INSTANCE.ring_buffer__poll(rb, timeoutMs);
        if (ret < 0) {
            throw new RuntimeException("ring_buffer__poll error: " + ret);
        }
        return ret;
    }

    public void close() {
        if (rb != null) {
            LibBpf.INSTANCE.ring_buffer__free(rb);
        }
    }
}
