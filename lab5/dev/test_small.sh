make clean
make
./gol 100 inputs/1k.pbm outputs/1k.pbm
# diff outputs/1k.pbm outputs/1k_verify_out.pbm