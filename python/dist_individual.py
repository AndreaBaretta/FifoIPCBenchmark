# Copyright © 2021 Andrea Baretta

# This file is part of FifoIPCLatency.

# FifoIPCLatency is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# FifoIPCLatency is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with FifoIPCLatency.  If not, see <https://www.gnu.org/licenses/>.

import numpy as np
import pandas as pd
import os
import matplotlib.pyplot as plt
import seaborn as sns
import scipy
import sys

dir_path = os.path.dirname(os.path.realpath(__file__))

cores = [0,9]

df = []
f = []

if len(sys.argv) == 1:
    print("Using default directory")
    df = pd.read_csv(dir_path + "/../builds/release/data/FifoIpcLatency_" + str(cores[0]) + "_" + str(cores[1]) + ".csv")
    f = open(dir_path + "/../builds/release/data/sysinfo.txt", "r").read()
else:
    print("Used specified directory: " + sys.argv[1])
    df = pd.read_csv(sys.argv[1] + "/FifoIpcLatency_" + str(cores[0]) + "_" + str(cores[1]) + ".csv")
    f = open(sys.argv[1] + "/sysinfo.txt", "r").read()

name = f.split("\n")[13][33:]

data = df["thread_1_round_trip_nano"]
data2 = df["thread_2_round_trip_nano"]

data = data / 2
data2 = data2 / 2

# data = data[1000:]
data = [x for x in data if x < 500]
data2 = [x for x in data2 if x < 500]

print("data1 max: ", np.max(data))
print("data2 max:", np.max(data2))

test = [x for x in data2 if x > 500]
print("Len test:", len(test))

ax = sns.distplot(data, hist=False, kde=True, 
    bins=int(80), color = 'blue',
    hist_kws={'edgecolor':'black'}, kde_kws={"bw":0.5, "gridsize":500}, label="Thread 1 Time")

sns.distplot(data2, hist=False, kde=True, 
    bins=int(80), color = 'red',
    hist_kws={'edgecolor':'black'}, kde_kws={"bw":0.5, "gridsize":500}, label="Thread 2 Time")

ax.set_title(name + ": Kernel Density Plot, Cores: " + str(cores[0]) + ", " + str(cores[1]))
ax.set(xlabel="Latency (ns)", ylabel="Density")

values = {}
for x in data:
    if x in values.keys():
        values[x] += 1
    else: 
        values[x] = 1

print(values)

print("Print sorted values")
print(sorted(values.items()))

ax.legend()
plt.show()