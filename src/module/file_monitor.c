#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/path.h>
#include <linux/namei.h>
#include <linux/sched/signal.h>
#include <linux/fdtable.h>
#include <linux/rcupdate.h>
#include <linux/sched/mm.h>
#include <linux/cred.h>
#include <linux/mm.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Aleksandar Djordjevic");
MODULE_DESCRIPTION("LKM File Monitor via SysFS Interface");

static struct kobject *file_monitor_kobj;
static char target_path_buffer[256];

static void print_proc_info(struct task_struct *task) {
    long rss = 0;
    unsigned long long total_time_ns = task->utime + task->stime;
    unsigned long long total_time_ms = total_time_ns / 1000000;
    unsigned int sec = total_time_ms / 1000;
    unsigned int msec = total_time_ms % 1000;
    
    if (task->mm) {
        rss = get_mm_rss(task->mm) << (PAGE_SHIFT - 10);
    }

    pr_info("FileMonitor: Pronadjen proces:\n");
    pr_info("  - Ime: %s\n", task->comm);
    pr_info("  - PID: %d | PPID: %d\n", task->pid, task_ppid_nr(task));
    pr_info("  - Stanje: %ld\n", task->__state);
    pr_info("  - Prioritet: %d\n", task->prio);
    pr_info("  - Nice: %d\n", task_nice(task));
    
    // Sigurno dohvatanje kredencijala
    rcu_read_lock();
    pr_info("  - Korisnik (UID): %u\n", from_kuid(&init_user_ns, __task_cred(task)->uid));
    pr_info("  - GID: %u\n", from_kgid(&init_user_ns, __task_cred(task)->gid));
    rcu_read_unlock();
    
    pr_info("  - Memorija: %ld KB\n", rss);
    pr_info("  - Broj niti: %d\n", task->signal->nr_threads);
    pr_info("  - Vreme izvrsenja: %u.%03u s\n", sec, msec);
    pr_info("--------------------------------------------------\n");
}

// Ova funkcija se automatski okida kada bilo ko upiše putanju u /sys/kernel/file_monitor/target_path
static ssize_t target_path_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    struct path path;
    struct inode *target_inode;
    struct task_struct *p;

    // Kopiramo putanju koju je korisnik poslao i uklanjamo novi red (\n)
    snprintf(target_path_buffer, sizeof(target_path_buffer), "%s", buf);
    strim(target_path_buffer);
    pr_info("[FileMonitor] Pokrenuta pretraga za fajl: %s\n", target_path_buffer);

    // Dobijamo Inode na osnovu prosleđene putanje
    if (kern_path(target_path_buffer, LOOKUP_FOLLOW, &path)) {
        pr_err("[FileMonitor] Neuspesno pronalazenje fajla na putanji: %s\n", target_path_buffer);
        return count;
    }
    target_inode = d_inode(path.dentry);

    rcu_read_lock();
    for_each_process(p) {
        struct files_struct *files;
        task_lock(p);
        files = p->files;
        if (files) {
            int fd;
            struct fdtable *fdt = files_fdtable(files);
        
            for (fd = 0; fd < fdt->max_fds; fd++) {
                struct file *file = rcu_dereference(fdt->fd[fd]);
                if (file && file->f_inode == target_inode) {
                    print_proc_info(p);
                    break;
                }
            }
        }
        task_unlock(p);
    }
    rcu_read_unlock();
    path_put(&path);
    return count;
}

// omogucava citanje trenutno postavljene putanje

static ssize_t target_path_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%s\n", target_path_buffer);
}

static struct kobj_attribute path_attribute = __ATTR(target_path, 0644, target_path_show, target_path_store);

static int __init file_monitor_init(void) {
    int error = 0;

    // Kreiranje direktorijuma /sys/kernel/file_monitor
    file_monitor_kobj = kobject_create_and_add("file_monitor", kernel_kobj);
    if (!file_monitor_kobj)
        return -ENOMEM;

    // Kreiranje SysFS fajla target_path unutar tog direktorijuma
    error = sysfs_create_file(file_monitor_kobj, &path_attribute.attr);
    if (error) {
        kobject_put(file_monitor_kobj);
        return error;
    }

    pr_info("[FileMonitor] Modul je uspesno ucitan u kernel i aktivan na /sys/kernel/file_monitor/target_path\n");
    return 0;
}

static void __exit file_monitor_exit(void) {
    kobject_put(file_monitor_kobj);
    pr_info("[FileMonitor] Modul je uklonjen iz kernela.\n");
}

module_init(file_monitor_init);
module_exit(file_monitor_exit);