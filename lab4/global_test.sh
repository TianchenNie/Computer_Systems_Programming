randtrack_global_lock 1 50 > rtgl.out
sort -n rtgl.out > rtgl.outs
randtrack 1 50 > rt.out
sort -n rt.out > rt.outs
diff rt.outs rtgl.outs

randtrack_global_lock 2 50 > rtgl.out
sort -n rtgl.out > rtgl.outs
randtrack 2 50 > rt.out
sort -n rt.out > rt.outs
diff rt.outs rtgl.outs

randtrack_global_lock 4 50 > rtgl.out
sort -n rtgl.out > rtgl.outs
randtrack 4 50 > rt.out
sort -n rt.out > rt.outs
diff rt.outs rtgl.outs