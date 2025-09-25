package com.unicamp.ebpfagent;

import com.sun.jna.Callback;
import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.IntByReference;

public interface LibBpf extends Library {

    // Carrega a biblioteca "bpf" (libbpf.so)
    LibBpf INSTANCE = Native.load("bpf", LibBpf.class);

    int bpf_map_lookup_elem(int fd, IntByReference key, LongByReference value);

    default long bpfMapLookupElem(int fd, int key) {
        LongByReference val = new LongByReference();
        IntByReference k = new IntByReference(key);
        int ret = bpf_map_lookup_elem(fd, k, val);
        if (ret != 0) return -1;
        return val.getValue();
    }


    int bpf_obj_get(String path);

    default int bpfObjGet(String path) {
        return bpf_obj_get(path);
    }

    // typedef int (*ring_buffer_sample_fn)(void *ctx, void *data, size_t size);
    interface RingBufferSampleFn extends Callback {
        int invoke(Pointer ctx, Pointer data, long size);
    }

    // struct ring_buffer *ring_buffer__new(int map_fd, ring_buffer_sample_fn fn, void *ctx, const struct ring_buffer_opts *opts);
    Pointer ring_buffer__new(int map_fd, RingBufferSampleFn fn, Pointer ctx, Pointer opts);

    // int ring_buffer__poll(struct ring_buffer *rb, int timeout_ms);
    int ring_buffer__poll(Pointer rb, int timeout_ms);

    // void ring_buffer__free(struct ring_buffer *rb);
    void ring_buffer__free(Pointer rb);
}
