# Copilot instructions for xv6-riscv-aging

Short, actionable guidance for AI coding agents working in this repository.

- Repo layout and big picture
  - kernel/: the kernel sources (C and .S). Key files: `kernel/proc.c`, `kernel/trap.c`, `kernel/syscall.c`, `kernel/vm.c`.
  - user/: user-space programs. Built into the filesystem image as binaries prefixed with `_` (e.g. `_sh`, `_ls`). `user/usys.pl` generates `usys.S` which is linked into user programs.
  - mkfs/: contains `mkfs` (mkfs/mkfs.c) used to create `fs.img` from `user/` binaries and `README`.
  - Makefile (root): central build rules. Variables: `K=kernel`, `U=user`, `SCHEDULER` (default RR). Toolchain prefix autodetected via `TOOLPREFIX`.

- Build, test and debug workflows (exact commands)
  - Build kernel and image: `make qemu` (invokes `make kernel/kernel` and `make fs.img`).
  - Build kernel only: `make kernel/kernel`.
  - Recreate filesystem image: `make fs.img` (runs `mkfs/mkfs fs.img README $(UPROGS)`).
  - Run QEMU (nographic): `make qemu`. Set `CPUS` or `SCHEDULER` like: `make CPUS=1 SCHEDULER=MLFQ qemu`.
  - Debug with gdb: `make qemu-gdb` (creates .gdbinit and prints a GDB port). Then run `gdb` in another terminal and connect to the printed port.
  - Automated tests: use `./test-xv6.py` (examples below).

- test-xv6.py examples (exact usage found in repo root)
  - Run all usertests: `./test-xv6.py usertests`.
  - Run quick usertests: `./test-xv6.py -q usertests`.
  - Run crash/recovery flows: `./test-xv6.py crash` or `./test-xv6.py log`.
  - The script uses `make qemu` to launch QEMU and controls the VM via stdin/stdout.

- Project-specific conventions and patterns
  - User programs are built into `_progname` binaries (UPROGS list in `Makefile`), then included in `fs.img` by `mkfs`.
  - Many CFLAGS disable standard library builtins (`-fno-builtin-*`). Avoid relying on host libc behaviour inside kernel code.
  - The build injects a scheduler macro into CFLAGS: `CFLAGS+="-D$(SCHEDULER)"`. To change scheduling behavior, set `SCHEDULER` when invoking make.
  - Kernel build artifacts: `kernel/kernel` (ELF), `kernel/kernel.asm` (disassembly), `kernel/kernel.sym` (symbol table).
  - GDB integration: `Makefile` generates `.gdbinit` from `.gdbinit.tmpl-riscv` and selects a dynamic port (GDBPORT). Use `make print-gdbport` to see the port.

- Integration points and external dependencies
  - Requires a RISC-V cross toolchain (riscv64-unknown-elf-*, or similar) in PATH.
  - Requires qemu-system-riscv64 >= 7.2 (Makefile checks version).
  - mkfs is built locally (mkfs/mkfs) and is used to build `fs.img`.
  - The QEMU command line references `fs.img` as a virtio block device: QEMU options live in `Makefile` (`QEMUOPTS`).

- Quick coding hints (concrete examples)
  - When adding a new user program, add its object to `UPROGS` (as `_name`) and ensure it links via `user/user.ld` and `usys.pl` if syscalls are used.
  - To add a new kernel source file, put it under `kernel/` and add its .o to the `OBJS` list in `Makefile`.
  - To debug a kernel panic or trace syscall flow, add printk-like messages in `kernel/printf.c` usage and reproduce via `./test-xv6.py` tests or manual `make qemu` then send commands to the kernel prompt.

- What to avoid / common pitfalls
  - Don’t assume host libc is available in kernel code—many builtins are disabled by CFLAGS.
  - Avoid changing the format of `fs.img` without adjusting `mkfs/mkfs.c` and the `Makefile` rules.
  - Changing `TOOLPREFIX` or toolchain can change object file formats; prefer using the toolchain the Makefile auto-detects when possible.

If anything in this guidance is incomplete or you want additional examples (e.g., a short contributor flow or a sample gdb session), tell me which area to expand and I will iterate.
