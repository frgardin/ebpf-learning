package com.unicamp.ebpfagent;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.ptr.LongByReference;
import com.sun.jna.ptr.IntByReference;

public interface LibBpf extends Library {

    // Carrega a biblioteca "bpf" (libbpf.so)
    LibBpf INSTANCE = Native.load("bpf", LibBpf.class);

    // Função da libbpf
    int bpf_map_lookup_elem(int fd, IntByReference key, LongByReference value);

    // Wrappers camelCase
    default long bpfMapLookupElem(int fd, int key) {
        LongByReference val = new LongByReference();
        IntByReference k = new IntByReference(key);
        int ret = bpf_map_lookup_elem(fd, k, val);
        if (ret != 0) return -1;
        return val.getValue();
    }
}
