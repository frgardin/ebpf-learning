package com.unicamp.ebpfagent;

import java.util.stream.IntStream;

public class EbpfAgent {
    public static void main(String[] args) throws Exception {
        int mapFd = LibBpf.INSTANCE.bpfObjGet("/sys/fs/bpf/execCounter");
        EbpfMapReader reader = new EbpfMapReader(mapFd);
        PrometheusExporter exporter = new PrometheusExporter();

        while (true) {
            IntStream.range(0,Integer.MAX_VALUE).forEach(pid -> {
                long count = reader.getCount(pid);
                if (count > 0) {
                    exporter.update(pid, count);
                }
            });
            Thread.sleep(2000);
        }
    }
}
