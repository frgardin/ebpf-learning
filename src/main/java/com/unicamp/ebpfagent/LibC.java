package com.unicamp.ebpfagent;

import com.sun.jna.Library;
import com.sun.jna.Native;
import com.sun.jna.Pointer;

public interface LibC extends Library {

    LibC INSTANCE = Native.load("c", LibC.class);

    int PROT_READ = 0x1;
    int PROT_WRITE = 0x2;
    int MAP_SHARED = 0x01;

    Pointer mmap(Pointer addr, long length, int prot, int flags, int fd, long offset);
    int munmap(Pointer addr, long length);

    int read(int fd, byte[] buffer, int count);
    int poll(PollFd[] fds, int nfds, int timeout);
}
