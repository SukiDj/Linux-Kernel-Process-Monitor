#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <linux/sched/mm.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/dcache.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandar");
MODULE_DESCRIPTION("Modul za pronalazenje procesa koji koriste odredjeni fajl");

static char *target_path = NULL;
module_param(target_path, charp, 0000);
MODULE_PARM_DESC(target_path, "Putanja do ciljanog fajla");

void print_process_info(struct task_struct *task) {
    long rss = 0;
    
    unsigned long long total_time_ns = task->utime + task->stime;
    unsigned long long total_time_ms = total_time_ns / 1000000;
    unsigned int sec = total_time_ms / 1000;
    unsigned int msec = total_time_ms % 1000;

    if (task->mm) {
        rss = get_mm_rss(task->mm) << (PAGE_SHIFT - 10);
    }

    pr_info("PRONADJEN PROCES:\n");
    pr_info("  - Ime: %s\n", task->comm);
    pr_info("  - PID: %d | PPID: %d\n", task->pid, task->real_parent->pid);
    pr_info("  - Stanje: %ld\n", task->__state);
    pr_info("  - Prioritet: %d\n", task->prio);
    pr_info("  - Nice: %d\n", task_nice(task));
    pr_info("  - Korisnik (UID): %u\n", from_kuid(&init_user_ns, task_cred_xxx(task, uid)));
    pr_info("  - GID: %u\n", from_kgid(&init_user_ns, task_cred_xxx(task, gid)));
    pr_info("  - Memorija (RSS): %ld KB\n", rss);
    pr_info("  - Broj niti: %d\n", task->signal->nr_threads);
    pr_info("  - Vreme izvrsenja: %u.%03u s\n", sec, msec);
    pr_info("--------------------------------------------------\n");
}

static int __init monitor_init(void) {
    struct path path_struct;
    struct inode *target_inode;
    struct task_struct *task;
    struct file *f;
    int err;

    pr_info("File Monitor: Modul se ucitava...\n");

    if (target_path == NULL) {
        pr_err("File Monitor: Morate uneti putanju! Koristite: insmod file_monitor.ko target_path=\"/putanja\"\n");
        return -1;
    }

    err = kern_path(target_path, LOOKUP_FOLLOW, &path_struct);
    if (err) {
        pr_err("File Monitor: Greska pri trazenju fajla '%s'. Greska br: %d\n", target_path, err);
        return err;
    }

    target_inode = path_struct.dentry->d_inode;
    pr_info("File Monitor: Ciljani fajl '%s' pronadjen. Inode: %lu\n", target_path, target_inode->i_ino);

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
                    print_process_info(task);
                    break;
                }
            }
        }
        task_unlock(task);
    }
    rcu_read_unlock();

    path_put(&path_struct);
    return 0;
}

static void __exit monitor_exit(void) {
    pr_info("File Monitor: Modul uklonjen.\n");
}

module_init(monitor_init);
module_exit(monitor_exit);
