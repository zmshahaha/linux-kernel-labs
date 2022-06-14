#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/pid.h>

#define MAX_SIZE 128

static struct proc_dir_entry *proc_ent;
static char output[MAX_SIZE];
static int out_len;

static struct page* find_page(int pid, unsigned long addr)
{
    pgd_t* pgd;
    p4d_t* p4d;
    pud_t* pud;
    pmd_t* pmd;
    pte_t* pte;

    struct task_struct *ptask = pid_task(find_vpid(pid),PIDTYPE_PID);
    struct mm_struct *mm = ptask->mm;

    pgd = pgd_offset(mm, addr);
    if(pgd_none(*pgd) || pgd_bad(*pgd))
        goto out;
    p4d = p4d_offset(pgd,addr);
    if(p4d_none(*p4d) || p4d_bad(*p4d))
        goto out;
    pud = pud_offset(p4d, addr);
    if(pud_none(*pud) || pud_bad(*pud))
        goto out;
    pmd = pmd_offset(pud, addr);
    if(pmd_none(*pmd) || pmd_bad(*pmd))
        goto out;
    pte = pte_offset_kernel(pmd, addr);
    if(pte_none(*pte))
        goto out;

    return pfn_to_page(pte_pfn(*pte));
    out :
        return NULL;
}
static char read_vma(int pid, unsigned long addr)
{
    struct page *page;

    char *kpage;
    char *caddr;
    char c;

    page = find_page(pid,addr);
    if(page==NULL)
    {
        pr_info("read vma err\n");
        return;
    }
    kpage = kmap_local_page(page);
    caddr = kpage+(addr&(PAGE_SIZE-1));
    c=*caddr;
    kunmap_local(kpage);
    return c;
}

static void write_vma(int pid,unsigned long addr,char c)
{
    struct page *page;

    char *kpage;
    char *caddr;

    page = find_page(pid,addr);
    if(page==NULL)
    {
        pr_info("write vma err\n");
        return;
    }
    kpage = kmap_local_page(page);
    caddr = kpage+(addr&(PAGE_SIZE-1));
    *caddr=c;
    kunmap_local(kpage);
}

static ssize_t proc_read(struct file *fp, char __user *ubuf, size_t len, loff_t *pos)
{
    int count;

    if (out_len - *pos > len) {
        count = len;
    }
    else {
        count = out_len - *pos;
    }

    if (copy_to_user(ubuf, output + *pos, count)) return -EFAULT;
    *pos += count;

    return count; 
}

static ssize_t proc_write(struct file *fp, const char __user *ubuf, size_t len, loff_t *pos)
{
    // TODO: parse the input, read/write process' memory
    char user_data[MAX_SIZE] = {0};
    char command=0;
    int pid = 0;
    unsigned long addr = 0;
    char c=0;

    if (len >= MAX_SIZE)
    {
        pr_info("lenth out of bound\n");
        return -EFAULT;
    }
    if (copy_from_user(user_data, ubuf, len))
    {
        pr_info("copy_from_user err\n");
        return -EFAULT;
    }
    user_data[len] = '\0';
    sscanf(user_data, "%s %d %lx %hhd", &command, &pid, &addr, &c);

    if (command=='r')
    {
        pr_info("Recv request: read mem %d %lx\n",pid,addr);
        sprintf(output,"%d",(int)read_vma(pid,addr));
        out_len=strlen(output);
    }
    else if (command=='w') 
    {
        pr_info("Recv request: write mem %d %lx %hhd\n", pid, addr, c);
        write_vma(pid,addr,c);
        sprintf(output,"%d",(int)c);
        out_len=strlen(output);
    }
    else
    {
        pr_info("Unsupported command: %s\n", user_data);
    }

    return len;
}

static const struct proc_ops proc_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init mtest_init(void)
{
    proc_ent = proc_create("mtest", 0666, NULL, &proc_ops);
    if (!proc_ent)
    {
        proc_remove(proc_ent);
        pr_alert("Error: Could not initialize /proc/mtest\n");
        return -EFAULT;
    }
    pr_info("/proc/mtest created %p \n",output);
    return 0;
}

static void __exit mtest_exit(void)
{
    proc_remove(proc_ent);
    pr_info("/proc/mtest removed\n");
}

module_init(mtest_init);
module_exit(mtest_exit);
MODULE_LICENSE("GPL");