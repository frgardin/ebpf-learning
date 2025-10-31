#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>

char __license[] SEC("license") = "GPL";

struct cpu_event
{
    __u32 pid;
    __u32 tgid;
    __u64 timestamp;
    char comm[16];
    __u64 runtime_ns;
    __u32 cpu_id;
};

struct
{
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} cpu_rb SEC(".maps");

struct
{
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, __u32);
    __type(value, __u64);
} prev_task_runtime SEC(".maps");

static __always_inline __u64 get_task_runtime(struct task_struct *task)
{
    // Try different kernel versions for sum_exec_runtime
    __u64 runtime = 0;

    // Method 1: Direct access (modern kernels)
    bpf_core_read(&runtime, sizeof(runtime), &task->se.sum_exec_runtime);

    // If zero, try alternative methods
    if (runtime == 0)
    {
        // Method 2: Try sched_entity location (older kernels)
        struct sched_entity *se = NULL;
        bpf_core_read(&se, sizeof(se), &task->se);
        if (se)
        {
            bpf_core_read(&runtime, sizeof(runtime), &se->sum_exec_runtime);
        }
    }

    return runtime;
}

SEC("tp_btf/sched_switch")
int BPF_PROG(trace_sched_switch, bool preempt, struct task_struct *prev, struct task_struct *next)
{
    __u32 prev_pid = BPF_CORE_READ(prev, pid);
    __u32 prev_tgid = BPF_CORE_READ(prev, tgid);

    // Skip kernel threads
    if (prev_tgid == 0)
        return 0;

    __u64 *prev_runtime_ptr = bpf_map_lookup_elem(&prev_task_runtime, &prev_pid);
    if (!prev_runtime_ptr)
    {
        __u64 zero = 0;
        bpf_map_update_elem(&prev_task_runtime, &prev_pid, &zero, BPF_ANY);
        return 0;
    }

    __u64 current_runtime = get_task_runtime(prev);
    __u64 delta = current_runtime - *prev_runtime_ptr;

    struct cpu_event *event = bpf_ringbuf_reserve(&cpu_rb, sizeof(*event), 0);
    if (!event)
        return 0;

    event->pid = prev_pid;
    event->tgid = prev_tgid;
    event->timestamp = bpf_ktime_get_tai_ns();
    event->runtime_ns = delta;
    event->cpu_id = bpf_get_smp_processor_id();
    bpf_get_current_comm(&event->comm, sizeof(event->comm));

    bpf_ringbuf_submit(event, 0);

    return 0;
}