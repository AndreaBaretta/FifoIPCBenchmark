# Copyright © 2021 Andrea Baretta

# This file is part of FifoIPCBenchmark.

# FifoIPCBenchmark is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# FifoIPCBenchmark is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with FifoIPCBenchmark. If not, see <https://www.gnu.org/licenses/>.

import numpy as np
import pandas as pd
import os
import matplotlib.pyplot as plt
import sys

plt.rcParams.update({
    "text.usetex": True
})

dir_path = os.path.dirname(os.path.realpath(__file__))

df = []
f = []
if len(sys.argv) == 1:
    print("Using default directory")
    df = pd.read_csv(dir_path + "/../builds/release/data/FifoIpcLatency_avg.csv")
    f = open(dir_path + "/../builds/release/data/sysinfo.txt", "r").read()
else:
    print("Used specified directory: " + sys.argv[1])
    df = pd.read_csv(sys.argv[1] + "/FifoIpcLatency_avg.csv")
    f = open(sys.argv[1] + "/sysinfo.txt", "r").read()

name = f.split("\n")[13][33:]

print("Processor: " + name)

core_1 = df["thread_1_core"]
core_2 = df["thread_2_core"]
time_nano = df["avg_round_time_nano"]

x_axis = list(set(core_1))
y_axis = list(set(core_2))

print("Axes")
print(x_axis)
print(y_axis)

size_core_1 = len(x_axis)
size_core_2 = len(y_axis)

table = np.empty((size_core_1,size_core_2))

zeros = 0

for x in range(0,size_core_1): #core_1
    for y in range(0,size_core_2): #core_2
        if (y == x):
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

# fig.colorbar(im, ticks=[np.max(table), np.min(table)])

ax.set_title(name + ": Single message latency (ns)")
fig.tight_layout()
plt.show()

print(table)

