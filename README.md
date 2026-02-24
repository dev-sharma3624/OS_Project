# Project D

This project is a custom, bare-metal x86_64 Operating System written entirely from scratch. It bypasses traditional bootloaders like GRUB, utilizing a custom UEFI bootloader to transition directly into a 64-bit higher-half kernel. 

The primary goal of this architecture was to deeply understand hardware-software boundaries, memory management, context switching, and bare-metal device driver implementation without relying on any existing kernel bases or legacy abstractions.

## 🚀 Core Features & Architecture

**Booting & Memory Management**
* Boots via a custom UEFI bootloader (compiled with Clang), which retrieves the memory map and framebuffer before handing execution to the kernel.
* Implements a **Bitmap Physical Memory Manager** and **Level 4 Paging** for virtual memory.
* Custom Heap implementation for dynamic memory allocation.

**CPU & Process Management**
* Custom implementation of the Global Descriptor Table (GDT) and Interrupt Descriptor Table (IDT).
* Hardware interrupts managed natively via the legacy Programmable Interrupt Controller (PIC) and Programmable Interval Timer (PIT).
* Multitasking is driven by a **Round-Robin Preemptive Scheduler**, performing bare-metal context switching (saving/restoring `RSP` and `RIP` registers).
* Full **Ring 3 Isolation** for user-space execution, complete with a custom system call interface.

**Storage & File System**
* Custom-written **NVMe Driver** communicating directly with the hardware controller's submission and completion queues.
* **FAT32 File System** implemented on top of the NVMe driver, capable of parsing the FAT table, creating sub-directories, and reading/writing text files.

**Graphics**
* Bare-metal Framebuffer renderer utilizing custom PSF1 fonts to draw text directly to the screen pixels.

---

## 🗺️ Memory Map
This OS implements a standard x86_64 **Higher-Half Kernel** design. The kernel is linked to execute in the upper regions of the virtual address space, while user-space applications (Ring 3) are mapped to the lower regions. 

The physical load address (LMA) is set safely at the 128 MB mark (`0x8000000`) to avoid conflicts with UEFI boot services and memory-mapped IO.

| Virtual Address Range | Description | Privilege Level |
| :--- | :--- | :--- |
| `0x0000000000000000` - `0x00007FFFFFFFFFFF` | User Space (Ring 3 Processes) | Ring 3 |
| `[ Canonical Hole ]` | Non-canonical address gap (Hardware enforces #GP) | N/A |
| `0xFFFFFFFF80000000` - `_KernelEnd` | Kernel Text, Data, and BSS (`.text`, `.rodata`, etc.) | Ring 0 |

*(Note: The gap between the upper and lower halves utilizes the x86_64 Canonical Hole. Any erroneous user-space pointers falling into this range physically trigger a General Protection Fault (Interrupt `0x0D`), which the kernel catches to kill the misbehaving process.)*

---

## 💻 Interaction & Usage
Once the OS successfully boots (tested via QEMU), it drops the user into a custom Ring 3 user-space shell. This demonstrates that the system call interface, privilege isolation, and keyboard interrupts are functioning correctly.

* Type `help` to view a formatted list of all available commands.
* You can utilize the shell to interact with the underlying FAT32 file system (reading/writing files and traversing directories) seamlessly.

---

## 🚧 Known Limitations & Roadmap
To maintain the scope of a solo architecture project, several hardware implementations are currently limited by design:
* **NVMe Driver:** Strictly polling-based. It does not currently utilize hardware interrupts, PRP (Physical Region Page) chaining, or Scatter/Gather Lists (SGL).
* **FAT32 System:** Supports basic file operations but does not currently handle Long File Names (LFN) or in-place file modification.
* **Syscalls:** The system call interface is custom-built and is *not* POSIX compliant.
* **Multiprocessing:** The current scheduler operates on a single core; Symmetric Multiprocessing (SMP) is a future roadmap target.

---

## 🛠️ Build Instructions
This project uses a custom shell script to automate the compilation of the bare-metal flags and link the object files via the included `linker.ld` script.

**Toolchain Requirements:**
* **Clang:** Used for compiling the UEFI bootloader into EFI byte code.
* **GCC (`x86_64-elf-gcc`):** Cross-compiler used for the kernel.
* **NASM:** Used for architecture-specific assembly files.

**1. Compile the Bootloader:**
```bash
clang -target x86_64-pc-win32-coff \
      -ffreestanding \
      -fno-stack-protector \
      -fshort-wchar \
      -mno-red-zone \
      -I Bootloader/Structs \
      -c Bootloader/boot.c -o image/object/boot.o

lld-link /subsystem:efi_application \
         /entry:efi_main \
         /out:image/EFI/BOOT/BOOTX64.EFI \
         image/object/boot.o
```

**2. Make the build script executable:**
```bash
chmod +x build.sh
```

**3. Run the build script:**
```bash
./build.sh
```

**4. Create and Format the Mock NVMe Drive:**
Because the OS features a custom NVMe driver and FAT32 file system, QEMU requires a properly formatted raw image file (`nvme.img`) to act as the physical SSD. 

You will need `kpartx` and `dosfstools` installed on your host machine to format the loopback device.

```bash
# 1. Create a 64MB empty raw image file
dd if=/dev/zero of=nvme.img bs=1M count=64

# 2. Create a GPT partition table and a basic partition
parted -s nvme.img mklabel gpt
parted -s nvme.img mkpart primary fat32 1MiB 100%

# 3. Map the image to a loop device to format it
sudo kpartx -av nvme.img

# 4. Format the mapped partition as FAT32 (Usually maps to loop0p1)
sudo mkfs.fat -F 32 /dev/mapper/loop0p1

# 5. Unmap the loop device safely
sudo kpartx -dv nvme.img
```

**5. Run in QEMU:**
```bash
qemu-system-x86_64 \
  -serial stdio \
  -bios /usr/share/ovmf/OVMF.fd \
  -drive format=raw,file=fat:rw:image \
  -m 256 \
  -drive file=nvme.img,format=raw,if=none,id=nvm \
  -device nvme,serial=deadbeef,drive=nvm
```

---

## 📄 License
This project is open-source and available under the **MIT License**.