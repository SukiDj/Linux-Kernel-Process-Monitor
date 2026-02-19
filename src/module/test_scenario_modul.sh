TARGET="/home/aleksandar-dj/test.txt"
echo "Test" > $TARGET
echo "--- POKRECEM 3 PROCESA KOJI KORISTE ISTI FAJL ---"

tail -f $TARGET > /dev/null &
PID1=$!
tail -f $TARGET > /dev/null &
PID2=$!
tail -f $TARGET > /dev/null &
PID3=$!

sleep 0.5

echo "--- UCITAVAM MODUL ---"
sudo insmod ./file_monitor.ko target_path="$TARGET"

echo "--- PROVERI DMESG ISPIS ---"
sleep 1

echo "--- CISCENJE ---"
sudo rmmod file_monitor
kill $PID1 $PID2 $PID3
rm $TARGET
