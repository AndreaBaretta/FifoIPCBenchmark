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
    "font.size": 15
})

colors = ['ro', 'bo', 'go', 'co', 'mo', 'yo']

df_intraccx = []
df_intraccx_all = []
names = []
i_intra_ccx_set = set()
individual_labels = []
labels_intraccx = []
graphs_intraccx = []
graphs_tot_intraccx = []

fig, ax = plt.subplots()

print("Used specified directory: " + sys.argv[1])
for i in range(0, len(os.listdir(sys.argv[1]))):
    file = sys.argv[1] + "/" + os.listdir(sys.argv[1])[i] + "/data/intra_ccx.csv"
    if (os.path.isfile(file)):
        df_intraccx.append(pd.read_csv(file))
        df_intraccx_all.append(pd.read_csv(sys.argv[1] + "/" + os.listdir(sys.argv[1])[i] + "/data/intra_ccx_all.csv"))
        f = open(sys.argv[1] + "/" + os.listdir(sys.argv[1])[i] + "/data/sysinfo.txt", "r").read()
        names.append(f.split("\n")[13][33:])
        print("Detected CPU: " + names[-1])

        num_intra_ccx = len(set(df_intraccx[-1]['num_pairs']))
        cpu_labels = []
        j = 1

        indeces = []

        while j < num_intra_ccx:
            i_intra_ccx_set.add(j)
            cpu_labels.append(str(j))
            indeces.append(j)
            j *= 2

            # graphs_intraccx[-1].append(np.average(df_intraccx[-1]['result']))
            # graphs_tot_intraccx[-1].append(np.average(df_intraccx[-1]['result']) * j)

        cpu_labels.append(str(num_intra_ccx))
        i_intra_ccx_set.add(num_intra_ccx)
        indeces.append(num_intra_ccx)

        graphs_intraccx.append([])
        graphs_tot_intraccx.append([])

        for i in indeces:
            intra_ccx_data = df_intraccx[-1].loc[df_intraccx[-1]['num_pairs'] == i]
            graphs_intraccx[-1].append(np.average(intra_ccx_data['result']))
            print("Intra ccx multiple:", i)
            graphs_tot_intraccx[-1].append(i*np.average(intra_ccx_data['result']))

        print("graphs_intraccx[-1]:", graphs_intraccx[-1])

        print("num_intra_ccx", num_intra_ccx)

        print("df_intraccx_all['num_pairs'][0]", df_intraccx_all[-1]['num_pairs'][0])
        num_intra_ccx_all = int(df_intraccx_all[-1]['num_pairs'][0])
        cpu_labels.append(str(num_intra_ccx_all))
        i_intra_ccx_set.add(num_intra_ccx_all)

        graphs_intraccx[-1].append(np.average(df_intraccx_all[-1]['result']))
        graphs_tot_intraccx[-1].append(num_intra_ccx_all*np.average(df_intraccx_all[-1]['result']))

        individual_labels.append(cpu_labels)




i_intra_ccx = np.sort(list(i_intra_ccx_set))
for i in i_intra_ccx:
    labels_intraccx.append(str(i))

print("i_intra_ccx: ", i_intra_ccx)

print("labels_intraccx", labels_intraccx)
ax.set_xticklabels(labels_intraccx)
ax.set(ylabel="Messages/second on one pair of cores")
ax.set(xlabel="Pairs of cores communicating simultaneously")
plt.title("Per-core throughput measurements")
print("graphs_intraccx:", graphs_intraccx)
for i in range(0, len(names)):
    print("i =", i)
    print("individual_labels[i]:", individual_labels[i])
    print("graphs_intraccx[i]:", graphs_intraccx[i])
    print("names[i]:", names[i])
    plt.plot(individual_labels[i], graphs_intraccx[i], colors[i], label=names[i])
plt.legend()

plt.figure(1)
fig, ax = plt.subplots()
ax.set_xticklabels(labels_intraccx)
ax.set(ylabel="Messages/second on all cores")
ax.set(xlabel="Pairs of cores")
plt.title("Total throughput measurements")
for i in range(0, len(names)):
    plt.plot(individual_labels[i], graphs_tot_intraccx[i], colors[i], label=names[i])

plt.legend()
plt.show()