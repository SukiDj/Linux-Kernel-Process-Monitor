#include <stdio.h>
#include <linux/kernel.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#define __NR_sys_file_monitor 470

int main() {
    char *putanja = "/home/aleksandar-dj/test_syscall.txt";

    long int status;

    printf("Pozivam sistemski poziv file_monitor (broj %d)...\n", __NR_sys_file_monitor);

    status = syscall(__NR_sys_file_monitor, putanja);

    if (status == 0) {
        printf("Sistemski poziv uspesno izvrsen! Proveri dmesg za detalje.\n");
    } else {
        printf("Greska pri pozivu! Kod greske: %ld\n", status);
        printf("Opis greske: %s\n", strerror(errno));
    }

    return 0;
}
