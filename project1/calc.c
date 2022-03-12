#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define MAX_SIZE 128
#define ID "519030910182"

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
int out_len;

static ssize_t proc_read(struct file *fp, char __user *ubuf, size_t len, loff_t *pos)
{
    int tmp[MAX_SIZE], p = 0, i, j, num;
    char buf[MAX_SIZE];
    memset((void *)output, 0, sizeof(output));
    if (*pos > 0)
    {
        return 0;
    }
    if (len < MAX_SIZE)
    {
        return -EFAULT;
    }
    if (strcmp(operator, "add") == 0)
    {
        for (i = 0; i < ninp; ++i)
        {
            tmp[i] = operand1 + operand2[i];
        }
    }
    else if (strcmp(operator, "mul") == 0)
    {
        for (i = 0; i < ninp; ++i)
        {
            tmp[i] = operand1 * operand2[i];
        }
    }
    for (i = 0; i < ninp; ++i)
    {
        for (num = tmp[i]; num > 0; num /= 10, ++p)
        {
            memset((void *)buf, 0, sizeof(buf));
            for (j = 0; num > 0; num /= 10, ++j)
            {
                buf[j] = num % 10 + '0';
            }
            for (--j; j >= 0; --j)
            {
                output[p++] = buf[j];
            }
        }
        output[p++] = ',';
    }
    output[--p] = '\n';
    ++p;
    output[p++] = '\0';
    if (copy_to_user((void *)ubuf, (void *)output, p))
    {
        return -EFAULT;
    }
    *pos = p;
    return p;
}

static ssize_t proc_write(struct file *fp, const char __user *ubuf, size_t len, loff_t *pos)
{
    int num = 0, i;
    memset((void *)output, 0, sizeof(output));
    if (*pos > 0 || len > MAX_SIZE || copy_from_user(output, ubuf, len))
    {
        return -EFAULT;
    }
    for (i = 0; i < len - 1; ++i)
    {
        num *= 10;
        num += (output[i] - '0');
    }
    operand1 = num;
    return len;
}

static const struct proc_ops proc_ops = {
    .proc_read = proc_read,
    .proc_write = proc_write,
};

static int __init proc_init(void)
{
    proc_dir = proc_mkdir(ID, NULL);
    proc_ent = proc_create("calc", 0666, proc_dir, &proc_ops);
    if (proc_dir && proc_ent)
    {
        return 0;
    }
    else
    {
        proc_remove(proc_ent);
        return -ENOMEM;
    }
}

static void __exit proc_exit(void)
{
    proc_remove(proc_dir);
    proc_remove(proc_ent);
}

module_init(proc_init);
module_exit(proc_exit);
MODULE_LICENSE("GPL");