make clean
make
# for i in {1..30}
# do
#     /usr/bin/time -f "%e real" ./gol 10000 inputs/1k.pbm outputs/1k.pbm
#     diff outputs/1k.pbm outputs/1k_verify_out.pbm
#     echo "Done test $i"
# done
for i in {1..50}
do
    /usr/bin/time -f "%e real" ./gol 10000 inputs/1k.pbm outputs/1k.pbm
    diff outputs/1k.pbm outputs/1k_verify_out_original.pbm
done