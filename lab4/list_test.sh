randtrack_list_lock 4 50 > rtll.out
sort -n rtll.out > rtll.outs
randtrack 4 50 > rt.out
sort -n rt.out > rt.outs
diff rt.outs rtll.outs
echo "Test $i done"