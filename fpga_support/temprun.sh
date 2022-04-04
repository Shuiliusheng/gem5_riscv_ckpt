./build/simulator-MediumBoomConfig ./riscv-pk/build/pk ./ckptinfo/readinfo/readinfo.riscv ./ckptinfo/info/ckpt_40000_60000.info ./ckptinfo/info/ckpt_syscall_40000_60000.info ./ckptinfo/test/test1.riscv


# This emulator compiled with JTAG Remote Bitbang client. To enable, use +jtag_rbb_enable=1.
# Listening on port 34075
# [UART] UART0 is here (stdin/stdout).
# bbl loader
# memsize: 0x0000000040000000, free_pages: 0x0000000000000400, RISCV_PGLEVEL_BITS: 9
# mmap_max: 0x000000003fbe9000, brk_max: 0x000000003fbe9000, stack_size: 0x0000000000800000, stack_bottom: 0x000000003f3e9000, stack_top: 0x000000003fbe9000
# load segment 1, vaddr: 0x000000000006db28, pmemsz: 0x00000000000035f8, brkmin: 0x0000000000071120
# text segment info, addr: 0x200000, size: 0x5aa75, endaddr: 0x25aa75
# padded text segment info, addr: 0x200000, size: 0x5b000, endaddr: 0x25b000
# --- load elf: alloc text memory: (0x200000, 0x25b000) ---
# --- ecall replace in text segment --- 
# load executable segment, addr: 0x200378, size: 0x3c72e, end: 0x23caa6
# load executable segment, addr: 0x23caa6, size: 0x7b2, end: 0x23d258
# --- step 1, read npc: 0x20d3e6 ---
# --- step 2, read integer and float registers ---
# --- step 3, read memory range information and do map, range num: 5 ---
# map range: (0x25c000, 0x262000), mapped addr: 0x25c000
# map range: (0x2fbe8000, 0x2fbe9000), mapped addr: 0x2fbe8000
# --- step 4, read first load information and init these loads, load num: 54 ---
# --- step 5, read syscall information and map them to memory, totalcallnum: 2 ---
# syscallnum: 2, alloc_mem: (0x2000000 0x2001000)
# --- step 6, init npc to rtemp(5) and takeover_syscall addr to rtemp(6) ---
# --- step 789, save registers data of boot program, set testing program registers, start testing ---
# syscall num: 0x40
# write, p0-p2: 0x1, 0x2612e0, 0x3d; record p0-p2: 0x1, 0x2612e0, 0x3d
# record data: 40 41 42 43 44 45 46 47 48 49 50 51 52 53 54 55 56 57 58 59 
# syscall num: 0x40
# write, p0-p2: 0x1, 0x2612e0, 0x3d; record p0-p2: 0x1, 0x2612e0, 0x3d
# record data: 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 60 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 
# syscall is overflow, exit!
