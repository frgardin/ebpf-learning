#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char __license[] SEC("license") = "GPL";

struct memory_event {
    __u32 pid;
    __u32 tgid;
    __u64 timestamp;
    char comm[16];
    __u64 rss_kb;
    __u64 swap_kb;
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} memory_rb SEC(".maps");

// Trace mm_page_alloc for memory allocations
SEC("tracepoint/kmem/mm_page_alloc")
int trace_mm_page_alloc(struct trace_event_raw_mm_page_alloc *args) {
    __u32 pid = bpf_get_current_pid_tgid() >> 32;
    __u32 tgid = (__u32)bpf_get_current_pid_tgid();
    
    // Skip kernel threads
    if (tgid == 0) return 0;
    
    struct memory_event *event = bpf_ringbuf_reserve(&memory_rb, sizeof(*event), 0);
    if (!event) return 0;
    
    event->pid = pid;
    event->tgid = tgid;
    event->timestamp = bpf_ktime_get_ns();
    bpf_get_current_comm(&event->comm, sizeof(event->comm));
    
    // Calculate memory allocated (order is log2 of number of pages)
    // Each page is typically 4KB
    event->rss_kb = (1 << args->order) * 4;
    event->swap_kb = 0;
    
    bpf_ringbuf_submit(event, 0);
    return 0;
}

// Trace mm_page_free for memory deallocations
SEC("tracepoint/kmem/mm_page_free")
int trace_mm_page_free(struct trace_event_raw_mm_page_free *args) {
    __u32 pid = bpf_get_current_pid_tgid() >> 32;
    __u32 tgid = (__u32)bpf_get_current_pid_tgid();
    
    if (tgid == 0) return 0;
    
    struct memory_event *event = bpf_ringbuf_reserve(&memory_rb, sizeof(*event), 0);
    if (!event) return 0;
    
    event->pid = pid;
    event->tgid = tgid;
    event->timestamp = bpf_ktime_get_ns();
    bpf_get_current_comm(&event->comm, sizeof(event->comm));
    
    // Calculate memory freed
    event->rss_kb = (1 << args->order) * 4;
    event->swap_kb = 0;
    
    bpf_ringbuf_submit(event, 0);
    return 0;
}