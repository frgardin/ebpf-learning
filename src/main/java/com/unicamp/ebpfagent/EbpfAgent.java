package com.unicamp.ebpfagent;

public class EbpfAgent {
    public static void main(String[] args) throws Exception {
        int mapFd = 123; 
        EbpfMapReader reader = new EbpfMapReader(mapFd);
        PrometheusExporter exporter = new PrometheusExporter();

        while (true) {
           
            for (int pid : new int[]{1, 2, 3}) {
                long count = reader.getCount(pid);
                if (count > 0) {
                    exporter.update(pid, count);
                }
            }
            Thread.sleep(2000);
        }
    }
}
