/*
 * resmap_account.c
 *
 */

#include <linux/cred.h>
#include <linux/debugfs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cpu.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/security.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/swap.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mm_types.h>
#include <linux/mempolicy.h>
#include <linux/rmap.h>
#include <linux/userfaultfd_k.h>
#include <linux/hugetlb.h>
#include <linux/resmap_account.h>

#include <linux/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/tlb.h>
#include <asm/mmu_context.h>

#include "internal.h"

#define THRIDPART_APP_UID_LOW_LIMIT 10000UL

const char *resmap_item_name[] = {
	"resmap_action",
	"resmap_success",
	"resmap_fail",
	"resmap_texit",
};

int svm_oom_pid = -1;
unsigned long svm_oom_jiffies = 0;
unsigned long gpu_compat_high_limit_addr;
DEFINE_PER_CPU(struct resmap_event_state, resmap_event_states);

int rlimit_svm_log = 1;
module_param_named(rlimit_svm_log, rlimit_svm_log, int, 0644);

int reserved_area_enable = 1;
module_param_named(reserved_area_enable, reserved_area_enable, int, 0644);

/* Kui.Zhang@PSW.TEC.KERNEL.Performance, 2019/03/18,
 * init the reserve area
 */
void init_reserve_mm(struct mm_struct* mm)
{
	mm->reserve_vma = NULL;
	mm->reserve_mmap = NULL;
	mm->reserve_mm_rb = RB_ROOT;
	mm->reserve_map_count = 0;
	mm->do_reserve_mmap = 0;

}
EXPORT_SYMBOL(init_reserve_mm);

/* Kui.Zhang@TEC.Kernel.Performance, 2019/03/27,
 * create reserved area depend on do_reserve_mmap value,
 * but need check the env, only 32bit process can used reserved area
 */
extern int vm_fra_op_enabled;

static void sum_resmap_events(unsigned int *ret)
{
	int cpu;
	int i;

	memset(ret, 0, RESMAP_TEXIT * sizeof(unsigned int));

	for_each_online_cpu(cpu) {
		struct resmap_event_state *this = &per_cpu(resmap_event_states,
				cpu);

		for (i = 0; i < RESMAP_TEXIT; i++)
			ret[i] += this->event[i];
	}
}

void all_resmap_events(unsigned int *ret)
{
	get_online_cpus();
	sum_resmap_events(ret);
	put_online_cpus();
}
EXPORT_SYMBOL_GPL(all_resmap_events);

static int resmap_account_show(struct seq_file *s, void *unused)
{
	int i;
	unsigned int all_events[RESMAP_TEXIT];

	all_resmap_events(all_events);
	for (i = 0; i < RESMAP_TEXIT; i++)
		seq_printf(s, "%-18s %u\n", resmap_item_name[i], all_events[i]);
	return 0;
}

static int resmap_account_open(struct inode *inode, struct file *file)
{
	return single_open(file, resmap_account_show, NULL);
}

static const struct file_operations resmap_account_fops = {
	.owner = THIS_MODULE,
	.open = resmap_account_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init resmap_account_init(void)
{
	struct dentry *f = debugfs_create_file("resmap_account", 0444, NULL,
			NULL, &resmap_account_fops);
	if (!f)
		pr_warn("%s create resmap_account failed\n", __func__);
	else
		pr_info("%s create resmap_account passed\n", __func__);

	return 0;
}

device_initcall(resmap_account_init);
MODULE_LICENSE("GPL");
