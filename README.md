# NOS - Projekat 1: Modifikacija i Rebuild Linux Kernela (v6.18.9)
Ovaj projekat obuhvata proširenje funkcionalnosti Linux jezgra kroz dva pristupa: dodavanjem novog sistemskog poziva i kreiranjem kernel modula (LKM). Fokus je na dubinskoj analizi struktura procesa i identifikaciji vlasnika otvorenih datoteka.

## Teorijska osnova
Projekat se oslanja na koncepte iz knjige Linux Kernel Development (Robert Love):
* task_struct: Centralna struktura koja opisuje proces. Iz nje smo izvukli PID, prioritete, UID/GID i memorijske deskriptore.
* files_struct i fdtable: Korišćeni za iteraciju kroz otvorene file descriptore procesa.
* inode: Poređenjem inode broja zadate putanje i otvorenih fajlova vršena je identifikacija procesa.
* Vreme izvršenja: Izračunato kao suma utime (user) i stime (system) polja, konvertovanih iz nanosekundi u čitljiv format.

## Šta je urađeno
1. Rebuild Kernela: Modifikovan i kompajliran kernel verzije 6.18.9.

2. Novi Sistemski Poziv:

    *  Registrovan u syscall_64.tbl.
    * Implementiran u kernel/file_monitor.c koristeći SYSCALL_DEFINE1.
    * Omogućena bezbedna komunikacija user-kernel space-a preko strncpy_from_user.

3. Kernel Modul:

    * Napisan modul koji prihvata putanju fajla kao parametar (target_path).
    * Implementirana funkcija za iteraciju kroz sve procese u sistemu pomoću for_each_process.

4. Ekstrakcija metapodataka: Pored osnovnih zahteva, dodati su PPID, GID, broj niti i precizno CPU vreme.

## Ključne komande (Workflow)
1. Kompajliranje i instalacija kernela
```bash
# Kompajliranje jezgra (korišćenjem svih jezgara procesora)
make -j$(nproc)

# Instalacija modula i samog kernela
sudo make modules_install
sudo make install

# Popravka boot-a (generisanje initramfs slike i ažuriranje GRUB-a)
sudo update-initramfs -c -k 6.18.9
sudo update-grub
sudo reboot
```
2. Rad sa kernel modulom
```bash
# Kompajliranje modula preko Makefile-a
make

# Učitavanje modula sa putanjom do ciljanog fajla
sudo insmod file_monitor.ko target_path="/home/aleksandar-dj/test.txt"

# Provera logova (ovde se vide rezultati)
sudo dmesg | tail -n 50

# Uklanjanje modula
sudo rmmod file_monitor
```
3. Čišćenje sistema pre testiranja
```bash
# Ubijanje zaostalih demo procesa
sudo pkill tail

# Brisanje kernel logova radi lakšeg čitanja
sudo dmesg -c > /dev/null
```
## Testiranje
Testiranje je izvršeno na oba načina pomoću automatizovanih skripti i namenskih aplikacija:

* Za sistemski poziv: Napisana C aplikacija test_app.c koja poziva syscall i prosleđuje putanju.

* Za modul: Napisana Bash skripta test_scenario_modul.sh koja:

    * Kreira test fajl.
    * Pokreće više pozadinskih procesa (tail, sleep) koji drže taj fajl otvorenim.
    * Automatski učitava modul i prikazuje rezultate.