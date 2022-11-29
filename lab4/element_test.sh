for i in {0..100}
do
    randtrack_element_lock 4 50 > rtel.out
    sort -n rtel.out > rtel.outs
    randtrack 4 50 > rt.out
    sort -n rt.out > rt.outs
    diff rt.outs rtel.outs |& tee -a rtel_diff.txt
    echo "Test $i finished"
done