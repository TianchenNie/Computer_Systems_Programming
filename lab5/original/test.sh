make clean
make
# ./gol 10000 inputs/1k.pbm outputs/1k.pbm
./gol 10000 inputs/1k.pbm outputs/1k_verify_out.pbm
diff outputs/1k_verify_out.pbm outputs/1k.pbm