// ebpf/k8s_performance.bpf.c
#define BPF_NO_GLOBAL_DATA
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 4096);
} k8s_perf_events SEC(".maps");

struct k8s_perf_event {
    int pid;
    int uid;
    unsigned long long timestamp;
    int event_type;
    unsigned long long duration_ns;
    unsigned long long bytes;
    int syscall_id;
    char command[16];
    char message[32];
};

// Event types
#define EVENT_SYSCALL_ENTER   1
#define EVENT_SYSCALL_EXIT    2
#define EVENT_SCHED_IN        3
#define EVENT_SCHED_OUT       4
#define EVENT_NET_RX          5
#define EVENT_NET_TX          6
#define EVENT_FILE_READ       7
#define EVENT_FILE_WRITE      8

// Store syscall start times
struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, unsigned long long);
    __type(value, unsigned long long);
} syscall_start SEC(".maps");

// Define the tracepoint structures to avoid forward declaration issues
struct trace_event_raw_sys_enter {
    unsigned short common_type;
    unsigned char common_flags;
    unsigned char common_preempt_count;
    int common_pid;
    long id;
    unsigned long args[6];
};

struct trace_event_raw_sys_exit {
    unsigned short common_type;
    unsigned char common_flags;
    unsigned char common_preempt_count;
    int common_pid;
    long id;
    long ret;
};

SEC("tp/syscalls/sys_enter_openat")
int trace_openat_enter(struct trace_event_raw_sys_enter *ctx) {
    unsigned long long pid_tid = bpf_get_current_pid_tgid();
    unsigned long long start_time = bpf_ktime_get_ns();
    
    bpf_map_update_elem(&syscall_start, &pid_tid, &start_time, BPF_ANY);
    return 0;
}

SEC("tp/syscalls/sys_exit_openat")
int trace_openat_exit(struct trace_event_raw_sys_exit *ctx) {
    struct k8s_perf_event *event;
    unsigned long long pid_tid = bpf_get_current_pid_tgid();
    unsigned long long *start_time = bpf_map_lookup_elem(&syscall_start, &pid_tid);
    
    if (!start_time) return 0;
    
    event = bpf_ringbuf_reserve(&k8s_perf_events, sizeof(*event), 0);
    if (!event) {
        bpf_map_delete_elem(&syscall_start, &pid_tid);
        return 0;
    }
    
    event->pid = pid_tid >> 32;
    event->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    event->timestamp = bpf_ktime_get_ns();
    event->event_type = EVENT_SYSCALL_EXIT;
    event->duration_ns = event->timestamp - *start_time;
    event->syscall_id = 257; // openat
    event->bytes = ctx->ret > 0 ? ctx->ret : 0;
    bpf_get_current_comm(&event->command, sizeof(event->command));
    __builtin_memcpy(event->message, "OPENAT_COMPLETE", 16);
    
    bpf_ringbuf_submit(event, 0);
    bpf_map_delete_elem(&syscall_start, &pid_tid);
    return 0;
}

// Trace read operations
SEC("tp/syscalls/sys_enter_read")
int trace_read_enter(struct trace_event_raw_sys_enter *ctx) {
    unsigned long long pid_tid = bpf_get_current_pid_tgid();
    unsigned long long start_time = bpf_ktime_get_ns();
    
    bpf_map_update_elem(&syscall_start, &pid_tid, &start_time, BPF_ANY);
    return 0;
}

SEC("tp/syscalls/sys_exit_read")
int trace_read_exit(struct trace_event_raw_sys_exit *ctx) {
    struct k8s_perf_event *event;
    unsigned long long pid_tid = bpf_get_current_pid_tgid();
    unsigned long long *start_time = bpf_map_lookup_elem(&syscall_start, &pid_tid);
    
    if (!start_time) return 0;
    
    event = bpf_ringbuf_reserve(&k8s_perf_events, sizeof(*event), 0);
    if (!event) {
        bpf_map_delete_elem(&syscall_start, &pid_tid);
        return 0;
    }
    
    event->pid = pid_tid >> 32;
    event->uid = bpf_get_current_uid_gid() & 0xFFFFFFFF;
    event->timestamp = bpf_ktime_get_ns();
    event->event_type = EVENT_SYSCALL_EXIT;
    event->duration_ns = event->timestamp - *start_time;
    event->syscall_id = 0; // read
    event->bytes = ctx->ret > 0 ? ctx->ret : 0;
    bpf_get_current_comm(&event->command, sizeof(event->command));
    __builtin_memcpy(event->message, "READ_COMPLETE", 14);
    
    bpf_ringbuf_submit(event, 0);
    bpf_map_delete_elem(&syscall_start, &pid_tid);
    return 0;
}

char _license[] SEC("license") = "GPL";