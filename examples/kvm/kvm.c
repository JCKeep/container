#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <string.h>
#include <stdint.h>
#include <linux/kvm.h>

#define STACK_RSP       0x20000

#define PTE32_PRESENT   (1U << 0)   // 页表项存在位
#define PTE32_RW        (1U << 1)   // 读/写权限位
#define PTE32_USER      (1U << 2)   // 用户态访问权限位
#define PTE32_WRITETHRU (1U << 3)   // 写通透位
#define PTE32_CACHE     (1U << 4)   // 缓存控制位
#define PTE32_ACCESSED  (1U << 5)   // 访问位
#define PTE32_DIRTY     (1U << 6)   // 脏页位
#define PTE32_GLOBAL    (1U << 8)   // 全局页位

/* CR0 bits */
#define CR0_PE 1u
#define CR0_MP (1U << 1)
#define CR0_EM (1U << 2)
#define CR0_TS (1U << 3)
#define CR0_ET (1U << 4)
#define CR0_NE (1U << 5)
#define CR0_WP (1U << 16)
#define CR0_AM (1U << 18)
#define CR0_NW (1U << 29)
#define CR0_CD (1U << 30)
#define CR0_PG (1U << 31)

/* CR4 bits */
#define CR4_VME 1
#define CR4_PVI (1U << 1)
#define CR4_TSD (1U << 2)
#define CR4_DE (1U << 3)
#define CR4_PSE (1U << 4)
#define CR4_PAE (1U << 5)
#define CR4_MCE (1U << 6)
#define CR4_PGE (1U << 7)
#define CR4_PCE (1U << 8)
#define CR4_OSFXSR (1U << 8)
#define CR4_OSXMMEXCPT (1U << 10)
#define CR4_UMIP (1U << 11)
#define CR4_VMXE (1U << 13)
#define CR4_SMXE (1U << 14)
#define CR4_FSGSBASE (1U << 16)
#define CR4_PCIDE (1U << 17)
#define CR4_OSXSAVE (1U << 18)
#define CR4_SMEP (1U << 20)
#define CR4_SMAP (1U << 21)

#define EFER_SCE 1
#define EFER_LME (1U << 8)
#define EFER_LMA (1U << 10)
#define EFER_NXE (1U << 11)

/* 32-bit page directory entry bits */
#define PDE32_PRESENT 1
#define PDE32_RW (1U << 1)
#define PDE32_USER (1U << 2)
#define PDE32_PS (1U << 7)     // 使页表项指向4MB的大页，而不指向页表

/* 64-bit page * entry bits */
#define PDE64_PRESENT 1
#define PDE64_RW (1U << 1)
#define PDE64_USER (1U << 2)
#define PDE64_ACCESSED (1U << 5)
#define PDE64_DIRTY (1U << 6)
#define PDE64_PS (1U << 7)
#define PDE64_G (1U << 8)

struct vm {
	int sys_fd;
	int fd;
	char *mem;
};

void vm_init(struct vm *vm, size_t mem_size)
{
	int api_ver;
	struct kvm_userspace_memory_region memreg;

	vm->sys_fd = open("/dev/kvm", O_RDWR);
	if (vm->sys_fd < 0) {
		perror("open /dev/kvm");
		exit(1);
	}

	api_ver = ioctl(vm->sys_fd, KVM_GET_API_VERSION, 0);
	if (api_ver < 0) {
		perror("KVM_GET_API_VERSION");
		exit(1);
	}

	if (api_ver != KVM_API_VERSION) {
		fprintf(stderr, "Got KVM api version %d, expected %d\n",
			api_ver, KVM_API_VERSION);
		exit(1);
	}

	vm->fd = ioctl(vm->sys_fd, KVM_CREATE_VM, 0);
	if (vm->fd < 0) {
		perror("KVM_CREATE_VM");
		exit(1);
	}

	if (ioctl(vm->fd, KVM_SET_TSS_ADDR, 0xfffbd000) < 0) {
		perror("KVM_SET_TSS_ADDR");
		exit(1);
	}

	vm->mem = mmap(NULL, mem_size, PROT_READ | PROT_WRITE,
		       MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (vm->mem == MAP_FAILED) {
		perror("mmap mem");
		exit(1);
	}

	madvise(vm->mem, mem_size, MADV_MERGEABLE);

	memreg.slot = 0;
	memreg.flags = 0;
	memreg.guest_phys_addr = 0;
	memreg.memory_size = mem_size;
	memreg.userspace_addr = (unsigned long)vm->mem;
	if (ioctl(vm->fd, KVM_SET_USER_MEMORY_REGION, &memreg) < 0) {
		perror("KVM_SET_USER_MEMORY_REGION");
		exit(1);
	}
}

struct vcpu {
	int fd;
	struct kvm_run *kvm_run;
};

void vcpu_init(struct vm *vm, struct vcpu *vcpu)
{
	int vcpu_mmap_size;

	vcpu->fd = ioctl(vm->fd, KVM_CREATE_VCPU, 0);
	if (vcpu->fd < 0) {
		perror("KVM_CREATE_VCPU");
		exit(1);
	}

	vcpu_mmap_size = ioctl(vm->sys_fd, KVM_GET_VCPU_MMAP_SIZE, 0);
	if (vcpu_mmap_size <= 0) {
		perror("KVM_GET_VCPU_MMAP_SIZE");
		exit(1);
	}

	vcpu->kvm_run = mmap(NULL, vcpu_mmap_size, PROT_READ | PROT_WRITE,
			     MAP_SHARED, vcpu->fd, 0);
	if (vcpu->kvm_run == MAP_FAILED) {
		perror("mmap kvm_run");
		exit(1);
	}
}

int run_vm(struct vm *vm, struct vcpu *vcpu, size_t sz)
{
	struct kvm_regs regs;
	__attribute__((unused)) int32_t ret = 0;
	uint64_t memval = 0;

	for (;;) {
		if (ioctl(vcpu->fd, KVM_RUN, 0) < 0) {
			perror("KVM_RUN");
			exit(1);
		}

		switch (vcpu->kvm_run->exit_reason) {
		case KVM_EXIT_HLT:
			goto check;
		
		case KVM_EXIT_IO:
			if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_OUT
			    && vcpu->kvm_run->io.port == 0xE9) {
				char *p = (char *)vcpu->kvm_run;
				fwrite(p + vcpu->kvm_run->io.data_offset,
				       vcpu->kvm_run->io.size, 1, stdout);
				fflush(stdout);
				continue;
			} else if (vcpu->kvm_run->io.direction == KVM_EXIT_IO_IN
				&& vcpu->kvm_run->io.port == 0x80) {
				char *p = (char *)vcpu->kvm_run;
				ret = fread(p + vcpu->kvm_run->io.data_offset, 
					vcpu->kvm_run->io.size, 1, stdin);
				fflush(stdin);
				continue;
			}

				fflush(stdout);
			/* fall through */
		default:
			fprintf(stderr, "Got exit_reason %d,"
				" expected KVM_EXIT_HLT (%d)\n",
				vcpu->kvm_run->exit_reason, KVM_EXIT_HLT);
			exit(1);
		}
	}

      check:
	if (ioctl(vcpu->fd, KVM_GET_REGS, &regs) < 0) {
		perror("KVM_GET_REGS");
		exit(1);
	}

	if (regs.rax != 42) {
		printf("Wrong result: {E,R,}AX is %lld\n", regs.rax);
		return 0;
	}

	memcpy(&memval, &vm->mem[0x400], sz);
	if (memval != 42) {
		printf("Wrong result: memory at 0x400 is %lld\n",
		       (unsigned long long)memval);
		return 0;
	}

	return 1;
}

extern const unsigned char guest16[], guest16_end[];

int run_real_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing real mode\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	sregs.cs.selector = 0x0;
    /** 
     * 段基址设置为 0x100, kvm 虚拟机内物理地址从 0x100 开始
     * 实模式下，物理地址 = 段基址 + 偏移量
     * 实模式中访问 0x0 地址实际为 0x0 + 0x100 = 0x100
     */
	sregs.cs.base = 0x100;

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;

	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

#if 1
	int g16 = open("guest16.o", O_RDONLY);
	if (g16 < 0) {
		perror("open guest16.o");
		exit(1);
	}

	struct stat st;
	if (fstat(g16, &st) < 0) {
		perror("fstat guest16.o");
		exit(1);
	}
	
	char *g16_code = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, g16, 0);
	if (g16_code == MAP_FAILED) {
		perror("mmap guest16.o");
		exit(1);
	}

	memcpy(vm->mem, g16_code, st.st_size);
#else
	memcpy(vm->mem, guest16, guest16_end - guest16);
#endif

	return run_vm(vm, vcpu, 2);
}

static void setup_protected_mode(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0x0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11,	/* Code: execute, read, accessed */
		.dpl = 0,
		.db = 1,
		.s = 1,		/* Code/data */
		.l = 0,
		.g = 1,		/* 4KB granularity */
	};

	sregs->cr0 |= CR0_PE;	/* enter protected mode */

	sregs->cs = seg;

	seg.type = 3;		/* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

extern const unsigned char guest32[], guest32_end[];

int run_protected_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing protected mode\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	regs.rsp = STACK_RSP;

	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

#if 1
	int g32 = open("guest32.img", O_RDONLY);
	if (g32 < 0) {
		perror("open guest32.img.o");
		exit(1);
	}

	struct stat st;
	if (fstat(g32, &st) < 0) {
		perror("fstat guest32.img.o");
		exit(1);
	}
	
	char *g32_code = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, g32, 0);
	if (g32_code == MAP_FAILED) {
		perror("mmap guest32.img.o");
		exit(1);
	}

	memcpy(vm->mem, g32_code, st.st_size);
#else
	memcpy(vm->mem, guest32, guest32_end - guest32);
#endif

	return run_vm(vm, vcpu, 4);
}

/**
 * 在32位页表保护模式下，虚拟地址的转换过程如下：
 * 1. 根据段选择子选择相应的段进行访问。假设你选择了代码段。
 * 2. 在代码段的段描述符中，有一个基址（base）字段。将虚拟
 *    地址的偏移量加上基址，得到线性地址。
 * 3. 将线性地址的高 10 位作为页目录索引（PDE Index），中
 *    间 10 位作为页表索引（PTE Index），低 12 位作为页内
 *    偏移。
 * 4. 使用页目录索引找到页目录表中的对应项（页目录项）。页
 *    目录项的内容是指向页表的物理地址。
 * 5. 使用页表索引找到页表中的对应项（页表项）。页表项的内
 *    容是指向页面的物理地址。
 * 6. 将页表项中的物理地址与页内偏移相加，得到最终的物理地址。
 * 总结起来，虚拟地址转换的步骤为：段选择子选择段 -> 虚拟地
 * 址偏移加上基址得到线性地址 -> 线性地址划分为页目录索引、
 * 页表索引和页内偏移 -> 通过页目录索引找到页目录项 -> 通过
 * 页表索引找到页表项 -> 页表项中的物理地址与页内偏移相加得
 * 到物理地址。
 * 
 * 这是32位页表保护模式下的基本转换过程。具体实现会涉及到页
 * 目录表、页表的结构以及相关寄存器的设置等。
*/
static void setup_paged_32bit_mode(struct vm *vm, struct kvm_sregs *sregs)
{
	uint32_t pd_addr = 0x2000;
	uint32_t *pd = (void *)(vm->mem + pd_addr);

	/* A single 4MB page to cover the memory region */
	// pd[0] = PDE32_PRESENT | PDE32_RW | PDE32_USER | PDE32_PS;
	/* Other PDEs are left zeroed, meaning not present. */

	sregs->cr3 = pd_addr;
	sregs->cr4 = CR4_PSE;
	sregs->cr0
	    = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
	sregs->efer = 0;

	{
		// uint32_t pt_addr = 0x3000;
		// uint32_t *pt = (void *)(vm->mem + pt_addr);
		// uint32_t num_entries = (0x800 - 0x0) >> 12;

		// uint32_t virt_addr = 0x400;
    	// uint32_t phys_addr = 0x400;

		// for (uint32_t i = 0; i < num_entries; i++) {
		// 	uint32_t pt_index = virt_addr >> 12;	
		// 	pt[pt_index] = (phys_addr & 0xFFFFF000) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
		// 	virt_addr += 0x1000;
		// 	phys_addr += 0x1000;
    	// }

		// pd[0] = PDE32_PRESENT | PDE32_RW | PDE32_USER | (pt_addr & 0xfffff000);
		

		/* 虚拟地址通过页表转换后于物理地址相同 */
		uint32_t pt_addr = 0x3000;
		uint32_t *pt = (void *)(vm->mem + pt_addr);

		// Set up the page directory entry
		uint32_t pd_index = (0x00000000 >> 22) & 0x3FF;
		pd[pd_index] = (pt_addr & 0xFFFFF000) | PDE32_PRESENT | PDE32_RW | PDE32_USER;

		// Set up the page table entries
		for (int i = 0; i < 1024; i++) {
			pt[i] = (i << 12) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
		}

		/* 如果想虚拟地址 = 物理地址 + 0x1000 */
#if 0
		uint32_t pt_addr = 0x3000;
		uint32_t *pt = (void *)(vm->mem + pt_addr + 0x1000);

		// Set up the page directory entry
		uint32_t pd_index = (0x00000000 >> 22) & 0x3FF;
		pd[pd_index] = (((uint32_t)pt + 0x1000) & 0xFFFFF000) | PDE32_PRESENT | PDE32_RW | PDE32_USER;

		// Set up the page table entries
		for (int i = 0; i < 1024; i++) {
			pt[i] = (((i << 12) + 0x1000) & 0xFFFFF000) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
		}
#endif
	}
}

int run_paged_32bit_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing 32-bit paging\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_protected_mode(&sregs);
	setup_paged_32bit_mode(vm, &sregs);

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	regs.rsp = STACK_RSP;

	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

#if 1
	int g32 = open("guest32.img", O_RDONLY);
	if (g32 < 0) {
		perror("open guest32.img");
		exit(1);
	}

	struct stat st;
	if (fstat(g32, &st) < 0) {
		perror("fstat guest32.img");
		exit(1);
	}
	
	char *g32_code = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, g32, 0);
	if (g32_code == MAP_FAILED) {
		perror("mmap guest32.img");
		exit(1);
	}

	memcpy(vm->mem, g32_code, st.st_size);
#else
	memcpy(vm->mem, guest32, guest32_end - guest32);
#endif

	return run_vm(vm, vcpu, 4);
}

extern const unsigned char guest64[], guest64_end[];

static void setup_64bit_code_segment(struct kvm_sregs *sregs)
{
	struct kvm_segment seg = {
		.base = 0,
		.limit = 0xffffffff,
		.selector = 1 << 3,
		.present = 1,
		.type = 11,	/* Code: execute, read, accessed */
		.dpl = 0,
		.db = 0,
		.s = 1,		/* Code/data */
		.l = 1,
		.g = 1,		/* 4KB granularity */
	};

	sregs->cs = seg;

	seg.type = 3;		/* Data: read/write, accessed */
	seg.selector = 2 << 3;
	sregs->ds = sregs->es = sregs->fs = sregs->gs = sregs->ss = seg;
}

/**
 * 64位系统：
 * - `p4d_t`（Page Upper Directory）：PML4表，包含512个`p4d_entry`（PML4表项，最高9位）。
 * - `pud_t`（Page Upper Directory）：PDPT表，包含512个`pud_entry`（PDPT表项，9位）。
 * - `pmd_t`（Page Middle Directory）：页中间目录，包含512个`pmd_entry`（页中间目录项，9位）。
 * - `pte_t`（Page Table Entry）：页表，包含512个`pte_entry`（页表项，9位）。
 * 
 * 虚拟地址结构：
 * 
 * ``` 
 * 47      39   38          30   29          21   20        12   11           0
 * +----------+----------------+----------------+--------------+-------------+
 * |   PML4   |      PDPT      |  Page Middle   |  Page Table  |    Offset   |
 * +----------+----------------+----------------+--------------+-------------+
 * |   Index  |     Index      |      Index     |     Index    |             |
 * +----------+----------------+----------------+--------------+-------------+
 * ```
 */
static void setup_long_mode(struct vm *vm, struct kvm_sregs *sregs)
{
	uint64_t pml4_addr = 0x2000;
	uint64_t *pml4 = (void *)(vm->mem + pml4_addr);

	uint64_t pdpt_addr = 0x3000;
	uint64_t *pdpt = (void *)(vm->mem + pdpt_addr);

	uint64_t pd_addr = 0x4000;
	uint64_t *pd = (void *)(vm->mem + pd_addr);

	pml4[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pdpt_addr;
	pdpt[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | pd_addr;
	// pd[0] = PDE64_PRESENT | PDE64_RW | PDE64_USER | PDE64_PS;

	sregs->cr3 = pml4_addr;
	sregs->cr4 = CR4_PAE;
	sregs->cr0
	    = CR0_PE | CR0_MP | CR0_ET | CR0_NE | CR0_WP | CR0_AM | CR0_PG;
	sregs->efer = EFER_LME | EFER_LMA;

	/* 初始化 64 位页表，使虚拟地址 = 物理地址 */
	{
		uint64_t pt_addr = 0x5000;
		uint64_t *pt = (void *)(vm->mem + pt_addr);

		uint64_t pd_index = (0x0 >> 21) & 0x1FF;
		pd[pd_index] = (pt_addr & (~0xFFFUL)) | PDE64_PRESENT | PDE64_RW | PDE64_USER;

		for (uint64_t i = 0; i < 512; i++) {
			pt[i] = (i << 12) | PTE32_PRESENT | PTE32_RW | PTE32_USER;
		}
	}

	setup_64bit_code_segment(sregs);
}

int run_long_mode(struct vm *vm, struct vcpu *vcpu)
{
	struct kvm_sregs sregs;
	struct kvm_regs regs;

	printf("Testing 64-bit mode\n");

	if (ioctl(vcpu->fd, KVM_GET_SREGS, &sregs) < 0) {
		perror("KVM_GET_SREGS");
		exit(1);
	}

	setup_long_mode(vm, &sregs);

	if (ioctl(vcpu->fd, KVM_SET_SREGS, &sregs) < 0) {
		perror("KVM_SET_SREGS");
		exit(1);
	}

	memset(&regs, 0, sizeof(regs));
	/* Clear all FLAGS bits, except bit 1 which is always set. */
	regs.rflags = 2;
	regs.rip = 0;
	/* Create stack at top of 2 MB page and grow down. */
	regs.rsp = STACK_RSP;

	if (ioctl(vcpu->fd, KVM_SET_REGS, &regs) < 0) {
		perror("KVM_SET_REGS");
		exit(1);
	}

#if 1
	int g64= open("guest64.img", O_RDONLY);
	if (g64 < 0) {
		perror("open guest64.img");
		exit(1);
	}

	struct stat st;
	if (fstat(g64, &st) < 0) {
		perror("fstat guest64.img");
		exit(1);
	}
	
	char *g64_code = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, g64, 0);
	if (g64_code == MAP_FAILED) {
		perror("mmap guest32.img");
		exit(1);
	}

	memcpy(vm->mem, g64_code, st.st_size);
#else
	memcpy(vm->mem, guest64, guest64_end - guest64);
#endif

	return run_vm(vm, vcpu, 8);
}

int main(int argc, char **argv)
{
	struct vm vm;
	struct vcpu vcpu;
	enum {
		REAL_MODE,
		PROTECTED_MODE,
		PAGED_32BIT_MODE,
		LONG_MODE,
	} mode = REAL_MODE;
	int opt;

	while ((opt = getopt(argc, argv, "rspl")) != -1) {
		switch (opt) {
		case 'r':
			mode = REAL_MODE;
			break;

		case 's':
			mode = PROTECTED_MODE;
			break;

		case 'p':
			mode = PAGED_32BIT_MODE;
			break;

		case 'l':
			mode = LONG_MODE;
			break;

		default:
			fprintf(stderr, "Usage: %s [ -r | -s | -p | -l ]\n",
				argv[0]);
			return 1;
		}
	}

	vm_init(&vm, 0x200000);
	vcpu_init(&vm, &vcpu);

	switch (mode) {
	case REAL_MODE:
		return !run_real_mode(&vm, &vcpu);

	case PROTECTED_MODE:
		return !run_protected_mode(&vm, &vcpu);

	case PAGED_32BIT_MODE:
		return !run_paged_32bit_mode(&vm, &vcpu);

	case LONG_MODE:
		return !run_long_mode(&vm, &vcpu);
	}

	return 1;
}
