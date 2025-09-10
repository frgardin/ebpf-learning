package com.unicamp.ebpfagent;

import java.io.IOException;

public class EbpfAgent {
    public static void main(String[] args) throws IOException, Exception {
        int mapFd = LibBpf.INSTANCE.bpfObjGet("/sys/fs/bpf/execCounter");
        EbpfMapReader reader = new EbpfMapReader(mapFd);
        PrometheusExporter exporter = new PrometheusExporter();

        while (true) {
            for (int pid = 0; pid < Integer.MAX_VALUE; pid++) {
                long count = reader.getCount(pid);
                if (count > 0) {
                    Thread.sleep(1000);
                    System.out.println("pid= " + pid + "count= " +count);
                    exporter.update(pid, count);
                }
            }
            Thread.sleep(1000);
        }
    }
}
