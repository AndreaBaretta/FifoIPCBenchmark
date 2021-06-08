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
import sys

plt.rcParams.update({
    "text.usetex": True
})

dir_path = os.path.dirname(os.path.realpath(__file__))

df_intra_core = []
df_intra_ccx = []
df_intra_ccx_all = []
df_inter_ccx = []
df_inter_socket = []
f = []
if len(sys.argv) == 1:
    print("Using default directory")
    df_intra_core = pd.read_csv(dir_path + "/../builds/release/data/intra_core.csv")
    df_intra_ccx = pd.read_csv(dir_path + "/../builds/release/data/intra_ccx.csv")
    df_intra_ccx_all = pd.read_csv(dir_path + "/../builds/release/data/intra_ccx_all.csv")
    if (os.path.isfile(dir_path + "/../builds/release/data/inter_ccx.csv")):
        df_inter_ccx = pd.read_csv(dir_path + "/../builds/release/data/inter_ccx.csv")
    if (os.path.isfile(dir_path + "/../builds/release/data/inter_socket.csv")):
        df_inter_socket = pd.read_csv(dir_path + "/../builds/release/data/inter_socket.csv")
    f = open(dir_path + "/../builds/release/data/sysinfo.txt", "r").read()
else:
    print("Used specified directory: " + sys.argv[1])
    df_intra_core = pd.read_csv(sys.argv[1] + "/intra_core.csv")
    df_intra_ccx = pd.read_csv(sys.argv[1] + "/intra_ccx.csv")
    df_intra_ccx_all = pd.read_csv(sys.argv[1] + "/intra_ccx_all.csv")
    if (os.path.isfile(sys.argv[1] + "/inter_ccx.csv")):
        df_inter_ccx = pd.read_csv(sys.argv[1] + "/inter_ccx.csv")
    if (os.path.isfile(sys.argv[1] + "/inter_socket.csv")):
        df_inter_socket = pd.read_csv(sys.argv[1] + "/inter_socket.csv")
    f = open(sys.argv[1] + "/sysinfo.txt", "r").read()

name = f.split("\n")[13][33:]

print("Processor: " + name)

# Intra core
# plt.figure(1)
fig, ax = plt.subplots()
graphs = []
graphs_tot = []
graphs.append(df_intra_core["result"])

# Intra ccx
num_intra_ccx = len(set(df_intra_ccx['num_pairs']))
labels = ['Intra-core']
labels_tot = []
i = 1
i_intra_ccx = []
while i < num_intra_ccx:
    i_intra_ccx.append(i)
    labels.append('Intra-ccx ' + str(i))
    labels_tot.append('Intra-ccx ' + str(i))
    i *= 2
i_intra_ccx.append(num_intra_ccx)
labels.append('Intra-ccx ' + str(num_intra_ccx))
labels_tot.append('Intra-ccx ' + str(num_intra_ccx))
for i in i_intra_ccx:
    intra_ccx_data = df_intra_ccx.loc[df_intra_ccx['num_pairs'] == i]
    graphs.append(intra_ccx_data['result'])
    print("Intra ccx multiple:", i)
    graphs_tot.append(i*sum(intra_ccx_data['result'])/len(intra_ccx_data['result']))

# Intra ccx all
labels.append('Intra-ccx all')
labels_tot.append('Intra-ccx all')
graphs.append(df_intra_ccx_all['result'])
print("Intra ccx multiple:", df_intra_ccx_all['num_pairs'][0])
graphs_tot.append(df_intra_ccx_all['num_pairs'][0]*sum(df_intra_ccx_all['result'])/len(df_intra_ccx_all['result']))

# Inter ccx
if (len(df_inter_ccx) != 0):
    num_inter_ccx = len(set(df_inter_ccx['num_pairs']))
    i = 1
    i_inter_ccx = []
    while i < num_inter_ccx:
        i_inter_ccx.append(i)
        labels.append('Inter-ccx ' + str(i))
        labels_tot.append('Inter-ccx ' + str(i))
        i *= 2
    i_inter_ccx.append(num_inter_ccx)
    labels.append('Inter-ccx ' + str(num_inter_ccx))
    labels_tot.append('Inter-ccx ' + str(num_inter_ccx))
    for i in i_inter_ccx:
        inter_ccx_data = df_inter_ccx.loc[df_inter_ccx['num_pairs'] == i]
        graphs.append(inter_ccx_data['result'])
        print("Inter ccx multiple:", i)
        graphs_tot.append(i*sum(inter_ccx_data['result'])/len(inter_ccx_data['result']))

# Inter socket
if (len(df_inter_socket) != 0):
    print("Inter socket")
    num_inter_socket = len(set(df_inter_socket['num_pairs']))
    i = 1
    i_inter_socket = []
    while i < num_inter_socket:
        i_inter_socket.append(i)
        labels.append('Inter-socket ' + str(i))
        labels_tot.append('Inter-socket ' + str(i))
        i *= 2
    i_inter_socket.append(num_inter_socket)
    labels.append('Inter-socket ' + str(num_inter_socket))
    labels_tot.append('Inter-socket ' + str(num_inter_socket))
    for i in i_inter_socket:
        inter_socket_data = df_inter_socket.loc[df_inter_socket['num_pairs'] == i]
        graphs.append(inter_socket_data['result'])
        print("Inter ccx multiple:", i)
        graphs_tot.append(i*sum(inter_socket_data['result'])/len(inter_socket_data['result']))

# axis labels
ax.set_xticklabels(labels)
ax.set(ylabel="Messages/second on one pair of cores")

# Adding title
print('Labels: ', labels_tot)
plt.title("Throughput measurements: " + name)
plt.boxplot(graphs)

plt.figure(1)
fig, ax = plt.subplots()
ax.set_xticklabels(labels_tot)
ax.set(ylabel="Messages/second on all cores")
plt.title("Throughput measurements: " + name)
plt.plot(labels_tot, graphs_tot, 'ro')

plt.show()