package com.unicamp.ebpfagent;


public class EbpfAgent {

    public static void main(String[] args) throws Exception {
        int ringFd = LibBpf.INSTANCE.bpfObjGet("/sys/fs/bpf/output");
        System.out.println(ringFd);
        RingBufferReader ringReader = new RingBufferReader(ringFd, 4096);
        
        while (true) {
            ringReader.pollAndRead();

            Thread.sleep(1000);
        }
    }
}
