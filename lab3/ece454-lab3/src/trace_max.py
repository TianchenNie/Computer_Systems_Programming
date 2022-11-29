import os


max_mem = 0
for file in os.listdir("../traces"):
    with open("../traces/" + file, 'r') as f:
        lines = f.readlines()
        mem = lines[0]
        max_mem = max(max_mem, int(mem))

    f.close()

print(f"max mem: {max_mem} bytes.")
