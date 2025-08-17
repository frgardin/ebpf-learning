#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_execve")
int hello(struct trace_event_raw_sys_enter *ctx) {
    bpf_printk("Hello World!!\\n");
    return 0;
}

char LICENSE[] SEC("license") = "GPL";
