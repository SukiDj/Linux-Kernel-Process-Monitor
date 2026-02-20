TARGET="/home/aleksandar-dj/test_syscall.txt"
echo "Test podaci" > $TARGET

echo "--- POKRECEM 3 PROCESA KOJI KORISTE ISTI FAJL ---"

tail -f $TARGET > /dev/null &
PID1=$!
echo "Pokrenut proces 1 (PID: $PID1)"

tail -f $TARGET > /dev/null &
PID2=$!
echo "Pokrenut proces 2 (PID: $PID2)"

cat $TARGET > /dev/null &
sleep 50 < $TARGET &
PID3=$!
echo "Pokrenut proces 3 (PID: $PID3) - simulira dugo citanje"

sleep 1

echo "--- POZIVAM SISTEMSKI POZIV ---"
./test_app $TARGET

echo "--- GOTOVO! PROVERI DMESG ---"
echo "Cistim procese..."

kill $PID1 $PID2 $PID3
