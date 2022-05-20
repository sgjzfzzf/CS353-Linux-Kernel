#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/highmem.h>

#define MAX_SIZE 128

static struct proc_dir_entry *proc_ent;
static char output[MAX_SIZE];
static int out_len;

enum operation
{
    OP_READ,
    OP_WRITE
};

// Calculate the length of str.
int strlen_m(char *str)
{
    int count;
    count = 0;
    for (; str[count] != '\0' && str[count] != '\t' && str[count] != '\r' && str[count] != ' ' && str[count] != '\n'; ++count)
        ;
    return count;
}

// Convert string into ulong in 10 mode.
long atoi(char *str)
{
    int i, len, isneg;
    long sum;
    sum = 0;
    len = strlen_m(str);
    if (str[0] == '-')
    {
        isneg = 1;
    }
    else
    {
        isneg = 0;
    }
    for (i = isneg; i < len; ++i)
    {
        sum *= 10;
        if (str[i] >= '0' && str[i] <= '9')
        {
            sum += (str[i] - '0');
        }
        else
        {
            return -1;
        }
    }
    if (isneg)
    {
        sum = -sum;
    }
    return sum;
}

// Convert number into str with its length.
void itoa(long num, char *str, int *len)
{
    int i;
    long n;
    for (*len = 1, n = num; n >= 10; ++*len, n = n / 10)
        ;
    for (i = *len - 1, n = num; i >= 0; --i, n = n / 10)
    {
        str[i] = n % 10 + '0';
    }
    str[*len] = '\0';
}

// Convert string into ulong in 16 mode.
unsigned long atoh(char *str)
{
    int i, len;
    unsigned long sum;
    sum = 0;
    len = strlen_m(str);
    for (i = 0; i < len; ++i)
    {
        sum *= 0x10;
        if (str[i] >= '0' && str[i] <= '9')
        {
            sum += (str[i] - '0');
        }
        else if (str[i] >= 'a' && str[i] <= 'f')
        {
            sum += (str[i] - 'a' + 10);
        }
        else if (str[i] >= 'A' && str[i] <= 'F')
        {
            sum += (str[i] - 'A' + 10);
        }
        else
        {
            return -1;
        }
    }
    return sum;
}

// Get the pte of assigned process and its virtual address.
pte_t *get_pte(struct task_struct *task, unsigned long address)
{
    pgd_t *pgd;
    pud_t *pud;
    pmd_t *pmd;
    pte_t *pte;
    struct mm_struct *mm = task->mm;

    pgd = pgd_offset(mm, address);
    if (pgd_none(*pgd) || pgd_bad(*pgd))
        return NULL;

    pud = pud_offset((p4d_t *)pgd, address);
    if (pud_none(*pud) || pud_bad(*pud))
        return NULL;

    pmd = pmd_offset(pud, address);
    if (pmd_none(*pmd) || pmd_bad(*pmd))
        return NULL;

    pte = pte_offset_kernel(pmd, address);
    if (pte_none(*pte))
        return NULL;

    return pte;
}

static ssize_t proc_read(struct file *fp, char __user *ubuf, size_t len, loff_t *pos)
{
    int count; /* the number of characters to be copied */
    if (out_len - *pos > len)
    {
        count = len;
    }
    else
    {
        count = out_len - *pos;
    }

    pr_info("Reading the proc file\n");
    if (copy_to_user(ubuf, output + *pos, count))
        return -EFAULT;
    *pos += count;

    return count;
}

static ssize_t proc_write(struct file *fp, const char __user *ubuf, size_t len, loff_t *pos)
{
    // TODO: parse the input, read/write process' memory
    int i = 2, j, pid;
    char val;
    unsigned long vaddr;
    char kbuf[MAX_SIZE], op, *pidstr, *vaddrstr, *valstr;
    struct task_struct *task;
    struct page *page;
    pte_t *pte;
    void *paddr;
    if (copy_from_user(kbuf, ubuf, sizeof(kbuf)) || kbuf[1] != ' ')
    {
        return -EFAULT;
    }

    // Format data and convert them into suitable types.
    op = kbuf[0];
    pidstr = (char *)(kbuf + i);
    for (j = 0; i < len && kbuf[i] != ' ' && kbuf[i] != '\n' && kbuf[i] != '\0'; ++i, ++j)
        ;
    ++i;
    vaddrstr = (char *)(kbuf + i);
    for (j = 0; i < len && kbuf[i] != ' ' && kbuf[i] != '\n' && kbuf[i] != '\0'; ++i, ++j)
        ;
    pid = atoi(pidstr);
    vaddr = atoh(vaddrstr);
    if (op == 'w')
    {
        ++i;
        valstr = (char *)(kbuf + i);
        val = atoi(valstr);
    }

    // Get a map of the assigned address of the assigned task.
    if ((task = pid_task(find_pid_ns(pid, &init_pid_ns), PIDTYPE_PID)) == NULL)
    {
        return -EFAULT;
    }
    if ((pte = get_pte(task, vaddr)) == NULL)
    {
        return -EFAULT;
    }
    if ((page = pte_page(*pte)) == NULL)
    {
        return -EFAULT;
    }
    if ((paddr = kmap_local_page(page)) == NULL)
    {
        return -EFAULT;
    }
    // Release the map.
    kunmap(paddr);
    paddr = (void *)((unsigned long)paddr | (vaddr & ~PAGE_MASK));
    if (op == 'r')
    {
        itoa(*(char *)paddr, output, &out_len);
    }
    else if (op == 'w')
    {
        val = atoi(valstr);
        *(char *)paddr = val;
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
    pr_info("/proc/mtest created\n");
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