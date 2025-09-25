package com.unicamp.ebpfagent;

public class EbpfAgent {
    public static void main(String[] args) throws Exception {
        String pinPath = "/sys/fs/bpf/output"; // path onde o mapa foi pinned
        int mapFd = LibBpf.INSTANCE.bpfObjGet(pinPath);
        if (mapFd < 0) {
            throw new RuntimeException("bpf_obj_get failed for " + pinPath + " fd=" + mapFd);
        }

        RingBufferConsumer consumer = new RingBufferConsumer(mapFd);
        System.out.println("Ring buffer consumer created. Polling...");

        // loop de polling
        while (true) {
            // poll por até 1000ms. ret = number of events processed or 0 if timeout.
            int processed = consumer.poll(1000);
            // opcional: sleep ou outras tarefas
        }
        //consumer.close(); // unreachable here
    }
}
