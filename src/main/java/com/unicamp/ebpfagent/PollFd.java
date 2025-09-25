package com.unicamp.ebpfagent;

import com.sun.jna.Structure;
import java.util.List;
import java.util.Arrays;

public class PollFd extends Structure {
    public int fd;
    public short events;
    public short revents;

    @Override
    protected List<String> getFieldOrder() {
        return Arrays.asList("fd", "events", "revents");
    }
}
