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
    "text.usetex": True,
    "font.size": 13
})

colors = ['ro', 'bo', 'go', 'co', 'mo', 'yo']

df_intra_core = []
names = []
labels_intracore = ["Intra-core"]
graphs_intracore = []

print("Used specified directory: " + sys.argv[1])
for i in range(0, len(os.listdir(sys.argv[1]))):
    file = sys.argv[1] + "/" + os.listdir(sys.argv[1])[i] + "/data/intra_core.csv"
    if (os.path.isfile(file)):
        df_intra_core.append(pd.read_csv(file))
        f = open(sys.argv[1] + "/" + os.listdir(sys.argv[1])[i] + "/data/sysinfo.txt", "r").read()
        names.append(f.split("\n")[13][33:])
        print("Detected CPU: " + names[-1])

num_graphs_intracore = len(df_intra_core)
for i in df_intra_core:
    graphs_intracore.append(i)

print("CPUs: " + str(names))

fig, ax = plt.subplots()
ax.set_xticklabels(labels_intracore)
ax.set(ylabel="Messages/second on one pair of cores")

plt.title("Per-core throughput measurements")

for i in range(0, num_graphs_intracore):
    plt.plot(labels_intracore, [np.average(graphs_intracore[i]['result'])], colors[i], label=names[i])

plt.legend()
plt.show()