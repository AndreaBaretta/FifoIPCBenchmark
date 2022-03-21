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

colors = ['ro', 'bo', 'go', 'co', 'mo', 'yo']

df_intersocket = []
names = []
i_inter_socket_set = set()
individual_labels = []
labels_intersocket = []
graphs_intersocket = []
graphs_tot_intersocket = []

fig, ax = plt.subplots()

print("Used specified directory: " + sys.argv[1])
for i in range(0, len(os.listdir(sys.argv[1]))):
    file = sys.argv[1] + "/" + os.listdir(sys.argv[1])[i] + "/data/inter_socket.csv"
    if (os.path.isfile(file)):
        df_intersocket.append(pd.read_csv(file))
        f = open(sys.argv[1] + "/" + os.listdir(sys.argv[1])[i] + "/data/sysinfo.txt", "r").read()
        names.append(f.split("\n")[13][33:])
        print("Detected CPU: " + names[-1])

        num_inter_socket = len(set(df_intersocket[-1]['num_pairs']))
        cpu_labels = []
        j = 1

        indeces = []

        while j < num_inter_socket:
            i_inter_socket_set.add(j)
            cpu_labels.append(str(j))
            indeces.append(j)
            j *= 2

            # graphs_intraccx[-1].append(np.average(df_intraccx[-1]['result']))
            # graphs_tot_intraccx[-1].append(np.average(df_intraccx[-1]['result']) * j)

        cpu_labels.append(str(num_inter_socket))
        i_inter_socket_set.add(num_inter_socket)
        individual_labels.append(cpu_labels)
        indeces.append(num_inter_socket)

        graphs_intersocket.append([])
        graphs_tot_intersocket.append([])

        for i in indeces:
            intra_socket_data = df_intersocket[-1].loc[df_intersocket[-1]['num_pairs'] == i]
            graphs_intersocket[-1].append(np.average(intra_socket_data['result']))
            print("Inter socket multiple:", i)
            graphs_tot_intersocket[-1].append(i*np.average(intra_socket_data['result']))

        print("graphs_interccx[-1]:", graphs_intersocket[-1])

        print("num_inter_ccx", num_inter_socket)

i_inter_ccx = np.sort(list(i_inter_socket_set))
for i in i_inter_ccx:
    labels_intersocket.append(str(i))

print("i_intra_ccx: ", i_inter_ccx)

print("labels_intraccx", labels_intersocket)
ax.set_xticklabels(labels_intersocket)
ax.set(ylabel="Messages/second on one pair of cores")
ax.set(xlabel="Pairs of cores communicating simultaneously")
plt.title("Per-core throughput measurements")
print("graphs_intraccx:", graphs_intersocket)
for i in range(0, len(names)):
    print("i =", i)
    print("individual_labels[i]:", individual_labels[i])
    print("graphs_intraccx[i]:", graphs_intersocket[i])
    print("names[i]:", names[i])
    plt.plot(individual_labels[i], graphs_intersocket[i], colors[i], label=names[i])
plt.legend()

plt.figure(1)
fig, ax = plt.subplots()
ax.set_xticklabels(labels_intersocket)
ax.set(ylabel="Messages/second on all cores")
ax.set(xlabel="Pairs of cores")
plt.title("Total throughput measurements")
for i in range(0, len(names)):
    plt.plot(individual_labels[i], graphs_tot_intersocket[i], colors[i], label=names[i])

plt.legend()
plt.show()