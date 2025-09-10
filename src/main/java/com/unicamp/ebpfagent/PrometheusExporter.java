package com.unicamp.ebpfagent;

import java.io.IOException;

import io.prometheus.client.CollectorRegistry;
import io.prometheus.client.Gauge;
import io.prometheus.client.exporter.PushGateway;

public class PrometheusExporter {
    private final PushGateway pg;
    private final CollectorRegistry registry;
    private final Gauge execGauge;

    public PrometheusExporter() throws Exception {
        // Expõe HTTP /metrics
        this.pg = new PushGateway("localhost:9091");
        this.registry = new CollectorRegistry();
        this.execGauge = Gauge.build()
                .name("execve_calls_total")
                .help("Total execve calls observed by eBPF")
                .labelNames("pid")
                .register(registry);
    }

    public void update(int pid, long count) throws IOException {
        execGauge.labels(String.valueOf(pid)).set(count);
        try {
            pg.pushAdd(registry, "ebpfagent");
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
}
