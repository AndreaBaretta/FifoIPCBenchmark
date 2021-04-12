import numpy as np
import pandas as pd
import os
import matplotlib.pyplot as plt

dir_path = os.path.dirname(os.path.realpath(__file__))

df = pd.read_csv(dir_path + "/../builds/release/data/FifoIpcLatency_avg.csv")

# print(df.to_string())

core_1 = df["thread_1_core"]
core_2 = df["thread_2_core"]
time_nano = df["avg_round_time_nano"]

table = np.empty((32,32))

x_axis = list(set(core_1))
y_axis = list(set(core_2))

print("Axes")
print(x_axis)
print(y_axis)

size_core_1 = len(x_axis)
size_core_2 = len(y_axis)

zeros = 0

for x in range(0,size_core_1): #core_1
    for y in range(0,size_core_2): #core_2
        # print("x=" + str(x) + "  y=" + str(y) + "  i=" + str(size_core_1*(x) + y))
        if (y == x):
            # table[y][x] = 0
            zeros += 1
        else:
            table[y][x] = time_nano[size_core_1*(x) + y - zeros]/2

fig, ax = plt.subplots()
im = ax.imshow(table, cmap = 'Reds')

ax.set_xticks(np.arange(len(x_axis)))
ax.set_yticks(np.arange(len(y_axis)))

ax.set_xticklabels(x_axis)
ax.set_yticklabels(y_axis)

for i in range(len(y_axis)):
    for j in range(len(x_axis)):
        text = ax.text(j, i, int(table[i, j]),
            ha="center", va="center", color="w")

fig.colorbar(im, ticks=[np.max(table), np.min(table)])

ax.set_title("One way communication latency (ns)")
fig.tight_layout()
plt.show()

print(table)

