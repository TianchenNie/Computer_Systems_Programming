import os
import subprocess
import pprint

pprint.pprint("Starting...")
runs = 5
# tests = ["randtrack", "randtrack_global_lock", "randtrack_list_lock", "randtrack_element_lock", "randtrack_reduction"]
# tests = ["randtrack_list_lock", "randtrack_element_lock"]
tests = ["randtrack_reduction"]
measurements = {}
for test in tests:
    for threads in ["1", "2", "4"]:
        total_time = 0
        for i in range(runs):
            proc = subprocess.Popen(["/usr/bin/time", test, threads, "50"], stderr=subprocess.PIPE)
            out = proc.stderr.read().decode()
            # print("OUT: ", out)

            index = out.index("elapsed") - 1
            time = ""
            while out[index] != ':':
                time = out[index] + time
                index -= 1

            time = float(time)
            total_time += time

        measurements[test + " " + threads] = total_time / runs


pprint.pprint(measurements)
