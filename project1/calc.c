#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>

#define MAX_SIZE 128
#define ID "519021910189"
#define procfs_name "calc"

static int operand1;
module_param(operand1, int, 0);
static char *operator;
module_param(operator, charp, 0);
static int operand2[MAX_SIZE];
static int ninp;
module_param_array(operand2, int, &ninp, 0);

static struct proc_dir_entry *proc_ent;
static struct proc_dir_entry *proc_dir;
static char output[MAX_SIZE];
static char buf[MAX_SIZE];

int out_len;

static ssize_t proc_read(struct file *fp, char __user *ubuf, size_t len, loff_t *pos)
{
    /* TODO */

    bool isadd=!strcmp(operator,"add");
    bool ismul=!strcmp(operator,"mul");
    int index=0,i=0;

    if(*pos>0)return 0;

    if(isadd)
    {
        for ( ; i < ninp-1; i++)
        {
            int result=operand2[i] + operand1;
            index+=sprintf(output+index,"%d",result);
            output[index]=',';
            index++;
        }
        index+=sprintf(output+index,"%d", operand2[ninp-1] + operand1);
    }
    else if(ismul)
    {
        for ( ; i < ninp-1; i++)
        {
            /* code */
            int result=operand2[i] * operand1;
            index+=sprintf(output+index,"%d",result);
            output[index]=',';
            index++;
        }
        index+=sprintf(output+index,"%d", operand2[ninp-1] * operand1);
    }

    output[index]='\n';
    output[index+1]='\0';
    index+=2;
    
    if (copy_to_user(ubuf, output, index)) return -EFAULT;
    *pos = index;
    return index;
}

static ssize_t proc_write(struct file *fp, const char __user *ubuf, size_t len, loff_t *pos)
{ 
    int i=0;
    if (copy_from_user(buf, ubuf, len)) return -EFAULT;
    buf[len]='\0';
    
    operand1=0;
    for(;i<len&&buf[i]>='0'&&buf[i]<='9';++i)
    {
        operand1*=10;
        operand1+=(buf[i]-'0');
    }
    pr_info("%d\n",operand1);
    return len;
}

static const struct proc_ops proc_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void)
{
    /* TODO */
    proc_dir = proc_mkdir(ID, NULL);
    if (NULL == proc_dir) { 
        proc_remove(proc_dir); 
        pr_alert("Error:Could not initialize /proc/%s\n", ID); 
        return -ENOMEM; 
    } 
    proc_ent = proc_create(procfs_name, 0644, proc_dir, &proc_ops);
    if (NULL == proc_ent) { 
        proc_remove(proc_ent); 
        pr_alert("Error:Could not initialize /proc/%s\n", procfs_name); 
        return -ENOMEM; 
    } 
    pr_info("/proc/%s/%s created\n", ID, procfs_name); 
    return 0; 
}

static void __exit proc_exit(void)
{
    /* TODO */
    proc_remove(proc_ent); 
    pr_info("/proc/%s/%s removed\n", ID, procfs_name); 
    proc_remove(proc_dir); 
    pr_info("/proc/%s removed\n", ID); 
}

module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");
