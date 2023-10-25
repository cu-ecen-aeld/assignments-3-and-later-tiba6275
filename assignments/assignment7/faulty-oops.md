#Oops and Analysis
###Referenced developer.arm.com

`echo "hello_world" > /dev/faulty`

Self-explanatory, tries to dereference a null pointer
```
Unable to handle kernel NULL pointer dereference at virtual address 0000000000000000
```

Exception class, part of Exception Syndrome Register, shows a data abort and translation fault.
**Unsure about the translation fault, technical reference manual doesn't give much information
other than section or page fault, which both state "This happens if bits [1:0] of the descriptor are both 0"**
```
Mem abort info:
  ESR = 0x96000045
  EC = 0x25: DABT (current EL), IL = 32 bits
  SET = 0, FnV = 0
  EA = 0, S1PTW = 0
  FSC = 0x05: level 1 translation fault
```

Data abort info, WnR = 1 shows that it was a write error.
```
Data abort info:
  ISV = 0, ISS = 0x00000045
  CM = 0, WnR = 1
user pgtable: 4k pages, 39-bit VAs, pgdp=0000000042106000
```

Shows that there was an attempted access to addr 0
```
[0000000000000000] pgd=0000000000000000, p4d=0000000000000000, pud=0000000000000000
Internal error: Oops: 96000045 [#1] SMP
```

All the modules linked
```
Modules linked in: hello(O) faulty(O) scull(O)
```

Details about CPU core (0), process ID(158), command (shell), and hardware where it occurred
```
CPU: 0 PID: 158 Comm: sh Tainted: G           O      5.15.18 #1
Hardware name: linux,dummy-virt (DT)
pstate: 80000005 (Nzcv daif -PAN -UAO -TCO -DIT -SSBS BTYPE=--)
```

Shows error happened in faulty_write function of faulty module
```
pc : faulty_write+0x14/0x20 [faulty]
```

Registers when error occurred
```
lr : vfs_write+0xa8/0x2b0
sp : ffffffc008d0bd80
x29: ffffffc008d0bd80 x28: ffffff80020d4c80 x27: 0000000000000000
x26: 0000000000000000 x25: 0000000000000000 x24: 0000000000000000
x23: 0000000040001000 x22: 000000000000000c x21: 00000055908a2a70
x20: 00000055908a2a70 x19: ffffff80020a9200 x18: 0000000000000000
x17: 0000000000000000 x16: 0000000000000000 x15: 0000000000000000
x14: 0000000000000000 x13: 0000000000000000 x12: 0000000000000000
x11: 0000000000000000 x10: 0000000000000000 x9 : 0000000000000000
x8 : 0000000000000000 x7 : 0000000000000000 x6 : 0000000000000000
x5 : 0000000000000001 x4 : ffffffc0006f7000 x3 : ffffffc008d0bdf0
x2 : 000000000000000c x1 : 0000000000000000 x0 : 0000000000000000
```

The call trace at time of error
```
Call trace:
 faulty_write+0x14/0x20 [faulty]
 ksys_write+0x68/0x100
 __arm64_sys_write+0x20/0x30
 invoke_syscall+0x54/0x130
 el0_svc_common.constprop.0+0x44/0xf0
 do_el0_svc+0x40/0xa0
 el0_svc+0x20/0x60
 el0t_64_sync_handler+0xe8/0xf0
 el0t_64_sync+0x1a0/0x1a4
```

Assembly instructions when error occurred
``` 
Code: d2800001 d2800000 d503233f d50323bf (b900003f) 
---[ end trace b7ea611fca4d40d6 ]---
```

