make clean
make
./gol 10 inputs/1k.pbm outputs/1k.pbm > out.txt
# diff outputs/1k.pbm outputs/1k_verify_out.pbm