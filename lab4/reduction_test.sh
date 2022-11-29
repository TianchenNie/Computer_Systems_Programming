for i in {0..20}
do
    randtrack_reduction 4 50 > rtrd.out
    sort -n rtrd.out > rtrd.outs
    randtrack 4 50 > rt.out
    sort -n rt.out > rt.outs
    diff rt.outs rtrd.outs
    echo "Test $i done"
done