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


