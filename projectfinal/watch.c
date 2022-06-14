#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sched/cputime.h>
#include <linux/string.h>

#define MAX_SIZE 128

static struct proc_dir_entry *proc_ent;
static char output[MAX_SIZE];
static int out_len;
static struct task_struct* taskp;
u64 utime,stime;
static int page_access;

u64 nsec_to_clock_t(u64 x)
{
#if (NSEC_PER_SEC % USER_HZ) == 0
	return div_u64(x, NSEC_PER_SEC / USER_HZ);
#elif (USER_HZ % 512) == 0
	return div_u64(x * USER_HZ / 512, NSEC_PER_SEC / 512);
#else
	/*
         * max relative error 5.7e-8 (1.8s per year) for USER_HZ <= 1024,
         * overflow after 64.99 years.
         * exact for HZ=60, 72, 90, 120, 144, 180, 300, 600, 900, ...
         */
	return div_u64(x * 9, (9ull * NSEC_PER_SEC + (USER_HZ / 2)) / USER_HZ);
#endif
}

void update_page_access(unsigned long addr) {
	pte_t * pte, pte_tmp;
	pgd_t * pgd = NULL;
	p4d_t * p4d = NULL;
	pud_t * pud = NULL;
	pmd_t * pmd = NULL;
	spinlock_t *ptl;

		pgd = pgd_offset(taskp->mm, addr);
		if (pgd_none(*pgd) || pgd_bad(*pgd)) {
			printk("vaddr 0x%lx pgd not present.\n", addr);
			goto next;
		}
		p4d = p4d_offset(pgd, addr);
        if (p4d_none(*p4d) || p4d_bad(*p4d)) {
			printk("vaddr 0x%lx p4d not present.\n", addr);
			goto next;
		}
		pud = pud_offset(p4d, addr);
        if (pud_none(*pud) || pud_bad(*pud)) {
			printk("vaddr 0x%lx pud not present.\n", addr);
			goto next;
		}	
		pmd = pmd_offset(pud, addr);
        if (pmd_none(*pmd) || pmd_bad(*pmd)) {
			printk("vaddr 0x%lx pmd not present.\n", addr);
			goto next;
		}
		pte = pte_offset_map_lock(taskp->mm, pmd, addr, &ptl);
		if (pte && pte_present(*pte) && pte_young(*pte)) {
			pte_tmp = *pte;
			pte_tmp = pte_mkold(pte_tmp);
			set_pte_at(taskp->mm, addr, pte, pte_tmp);
			page_access++;
		}
		pte_unmap_unlock(pte, ptl);
next:
    return;
}

void thread_group_cputime(struct task_struct *tsk, struct task_cputime *times)
{
	struct signal_struct *sig = tsk->signal;
	u64 utime, stime;
	struct task_struct *t;
	unsigned int seq, nextseq;
	unsigned long flags;

	/*
	 * Update current task runtime to account pending time since last
	 * scheduler action or thread_group_cputime() call. This thread group
	 * might have other running tasks on different CPUs, but updating
	 * their runtime can affect syscall performance, so we skip account
	 * those pending times and rely only on values updated on tick or
	 * other scheduler action.
	 */
	/* if (same_thread_group(current, tsk))
		(void) task_sched_runtime(current); */

	rcu_read_lock();
	/* Attempt a lockless read on the first round. */
	nextseq = 0;
	do {
		seq = nextseq;
		flags = read_seqbegin_or_lock_irqsave(&sig->stats_lock, &seq);
		times->utime = sig->utime;
		times->stime = sig->stime;
		times->sum_exec_runtime = sig->sum_sched_runtime;

		for_each_thread(tsk, t) {
			task_cputime(t, &utime, &stime);
			times->utime += utime;
			times->stime += stime;
		}
		/* If lockless access failed, take the lock. */
		nextseq = 1;
	} while (need_seqretry(&sig->stats_lock, seq));
	done_seqretry_irqrestore(&sig->stats_lock, seq, flags);
	rcu_read_unlock();
}

static ssize_t proc_read(struct file *fp, char __user *ubuf, size_t len, loff_t *pos)
{
    int count; /* the number of characters to be copied */
    u64 out_utime=utime,out_stime=stime;
    struct vm_area_struct *vma;
    unsigned long addr;
    struct task_cputime cputime;
    if (*pos == 0) {
        /* a new read, update process' status */
        /* TODO */
	    thread_group_cputime(taskp, &cputime);
	    utime = cputime.utime;
	    stime = cputime.stime;
        out_utime=utime-out_utime;
        out_stime=stime-out_stime;

        page_access=0;
        for(vma =taskp-> mm->mmap; vma; vma=vma->vm_next){
            for(addr=vma->vm_start;addr<vma->vm_end;addr+=PAGE_SIZE){
                update_page_access(addr);
            }
        }

        sprintf(output,"utime: %lld ,stime: %lld ,pages access: %d\n",nsec_to_clock_t(out_utime),nsec_to_clock_t(out_stime),page_access);
        out_len=strlen(output);
    }
    if (out_len - *pos > len) {
        count = len;
    } else {
        count = out_len - *pos;
    }

    pr_info("Reading the proc file\n");
    if (copy_to_user(ubuf, output + *pos, count)) return -EFAULT;
    *pos += count;
    
    return count;
}

static ssize_t proc_write(struct file *fp, const char __user *ubuf, size_t len, loff_t *pos)
{
    int pid;

    if (*pos > 0) return -EFAULT;
    pr_info("Writing the proc file\n");
    if(kstrtoint_from_user(ubuf, len, 10, &pid)) return -EFAULT;

    taskp = get_pid_task(find_get_pid(pid), PIDTYPE_PID);
    utime=0;
    stime=0;

    *pos += len;
    return len;
}

static const struct proc_ops proc_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init watch_init(void)
{
    proc_ent = proc_create("watch", 0666, NULL, &proc_ops);
    if (!proc_ent) {
        proc_remove(proc_ent);
        pr_alert("Error: Could not initialize /proc/watch\n");
        return -EFAULT;
    }
    pr_info("/proc/watch created\n");
    return 0;
}

static void __exit watch_exit(void)
{
    proc_remove(proc_ent);
    pr_info("/proc/watch removed\n");
}

module_init(watch_init);
module_exit(watch_exit);
MODULE_LICENSE("GPL");