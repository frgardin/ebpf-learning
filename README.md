-- Hello! Here I will display notes for EBPF!! --
-- Obviously, I hope that this archive get big enough to be separated in other achives lol -- 

# Liz Book - notes

## Chapter 1 - What is eBPF, and Why Is It Important?

- eBPF: Extended Berkeley Packet Filter.

- Definition:
    - eBPF is a revolutioanry kernel technologies that allows developers to write custom code that can be loaded into the kernel dynamically, changing the kernel behaves. This enables a new generation of highly performant networking, observability, and security tools. 
        - Performance tracing of pretty much any aspect of a system.
	- High-performance networking, with built-in visibility.
	- Detecting and (optionally) preventing malicious activity.

### eBPF's Roots: The Berkeley Packet Filter

- has its roots in the BSD Packet Filter, described in 1993, paper written by Lawrence Berkeley National Laboratory's Steven McCanne and Van Jacobson.
    - paper discuss pseudomachine that can run *filters*, which are programs to determine wheter to accept or reject a network packet.
    - programs were whitten in BPF instruction set, a general-purpose of 32-bit instructions that closely resembles assembly language.

Ex.

```assembly
ldh	[12]
jeq	#ETHERTYPE IP, L1, L2
L1:	ret	#TRUE
L2:	ret	#0
```

- This code filters out packets that are not Internet Protocol (IP) packets.
    - **ldh**= loads a 2-byte value starting at byte 12 in this packet.
    - **jeq**= compare the value that represents a IP packet
    - if last instruction is true, then jump to instruction labeled L1, where packet is accepted returning a non-zero value, else return 0

- BPF ("Berkeley Packet Filter"), introduced to Linux in 1997, in kernel version 2.1.75, where it was used in the tcpdump utility as an efficient way to capture the packets to be traced out.

- in 2012, when seccomp-bpf was introduced in version 3.5 of the kernel. This enabled the use of BPF programs to make decisions about wheter to allow or deny user space applications from making system calls. This was the first step in the evolving BPF from the narrow scope of packet filtering to the general-purpose platform it is today.

### From BPF to eBPF

- BPF evolved to waht we call "extended BPF" or "eBPF" starting in kernel version 3.18 in 2014. This involved several significant changes:
    - BPF instructions overhauled to be more efficient on 64-bit machines, and the interpreter was entirely rewritten.
    - eBPF *maps* were introduced: data structures that can be accessed by BPF programs and by user space applications, allowing information to be shared between them.
    - the bpf() syscall was added, so that user space programs can interact with eBPF programs in the kernel.
    - several BPF helpres functions were added.
    - the eBPF verifier was added to ensure that eBPF programs are safe to run.

 
