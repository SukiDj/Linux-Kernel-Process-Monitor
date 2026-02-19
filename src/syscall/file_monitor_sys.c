#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

void print_proc_info(struct task_struct *task) {
    long rss = 0;
    unsigned long long total_time_ns = task->utime + task->stime;
    unsigned long long total_time_ms = total_time_ns / 1000000;
    unsigned int sec = total_time_ms / 1000;
    unsigned int msec = total_time_ms % 1000;
    
    if (task->mm) {
        rss = get_mm_rss(task->mm) << (PAGE_SHIFT - 10);
    }

    pr_info("SYSCALL_MONITOR: Pronadjen proces:\n");
    pr_info("  - Ime: %s\n", task->comm);
    pr_info("  - PID: %d | PPID: %d\n", task->pid, task->real_parent->pid);
    pr_info("  - Stanje: %u\n", task->__state);
    pr_info("  - Prioritet: %d\n", task->prio);
    pr_info("  - Nice: %d\n", task_nice(task));
    pr_info("  - Korisnik (UID): %u\n", from_kuid(&init_user_ns, task_cred_xxx(task, uid)));
    pr_info("  - GID: %u\n", from_kgid(&init_user_ns, task_cred_xxx(task, gid)));
    pr_info("  - Memorija: %ld KB\n", rss);
    pr_info("  - Broj niti: %d\n", task->signal->nr_threads);
    pr_info("  - Vreme izvrsenja: %u.%03u s\n", sec, msec);
    pr_info("--------------------------------------------------\n");
}

SYSCALL_DEFINE1(file_monitor, char __user *, user_path)
{
    struct path path_struct;
    struct inode *target_inode;
    struct task_struct *task;
    struct file *f;
    char *kernel_path_buffer;
    int err;
    long copied;

    pr_info("SYSCALL_MONITOR: Pozvan sistemski poziv.\n");

    kernel_path_buffer = kmalloc(PATH_MAX, GFP_KERNEL);
    if (!kernel_path_buffer)
        return -ENOMEM;

    copied = strncpy_from_user(kernel_path_buffer, user_path, PATH_MAX);
    if (copied < 0) {
        kfree(kernel_path_buffer);
        return -EFAULT;
    }

    err = kern_path(kernel_path_buffer, LOOKUP_FOLLOW, &path_struct);
    if (err) {
        pr_err("SYSCALL_MONITOR: Greska pri trazenju fajla '%s'. Kod: %d\n", kernel_path_buffer, err);
        kfree(kernel_path_buffer);
        return err;
    }

    target_inode = path_struct.dentry->d_inode;
    pr_info("SYSCALL_MONITOR: Trazim procese za fajl: %s (Inode: %lu)\n", kernel_path_buffer, target_inode->i_ino);

    rcu_read_lock();
    for_each_process(task) {
        struct files_struct *files;

        task_lock(task);
        files = task->files;
        if (files) {
            struct fdtable *fdt = files_fdtable(files);
            int i;

            for (i = 0; i < fdt->max_fds; i++) {
                f = fdt->fd[i];
                if (f && f->f_inode == target_inode) {
                    print_proc_info(task);
                    break;
                }
            }
        }
        task_unlock(task);
    }
    rcu_read_unlock();

    path_put(&path_struct);
    kfree(kernel_path_buffer);

    return 0;
}
