package com.unicamp.ebpfagent;

public class EbpfMapReader {

    private int mapFd;

    public EbpfMapReader(int mapFd) {
        this.mapFd = mapFd;
    }

    public long getCount(int pid) {
        return LibBpf.INSTANCE.bpfMapLookupElem(mapFd, pid);
    }
}
