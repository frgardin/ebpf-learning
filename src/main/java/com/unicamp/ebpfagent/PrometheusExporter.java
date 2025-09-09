package com.unicamp.ebpfagent;

import io.prometheus.client.Counter;
import io.prometheus.client.exporter.HTTPServer;

public class PrometheusExporter {
    private Counter execCounter = Counter.build()
            .name("execve_calls_total")
            .help("Total execve calls observed by eBPF")
            .labelNames("pid")
            .register();

    public PrometheusExporter() throws Exception {
        // Expõe HTTP /metrics
        HTTPServer server = new HTTPServer(9100);
    }

    public void update(int pid, long count) {
        execCounter.labels(String.valueOf(pid)).inc(count);
    }
}
