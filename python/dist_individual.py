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
from matplotlib.ticker import AutoMinorLocator
from matplotlib.offsetbox import AnchoredText
import seaborn as sns
import scipy
import sys

plt.rcParams.update({
    "text.usetex": True
})

dir_path = os.path.dirname(os.path.realpath(__file__))

cores = [0,79]

df = []
df2 = []
f = []

if len(sys.argv) == 1:
    print("Using default directory")
    df = pd.read_csv(dir_path + "/../builds/release/data/FifoIpcLatency_" + str(cores[0]) + "_" + str(cores[1]) + ".csv")
    df2 = pd.read_csv(dir_path + "/../builds/release/data/FifoIpcLatency_period.csv")
    f = open(dir_path + "/../builds/release/data/sysinfo.txt", "r").read()
else:
    print("Used specified directory: " + sys.argv[1])
    df = pd.read_csv(sys.argv[1] + "/FifoIpcLatency_" + str(cores[0]) + "_" + str(cores[1]) + ".csv")
    df2 = pd.read_csv(sys.argv[1] + "/FifoIpcLatency_period.csv")
    f = open(sys.argv[1] + "/sysinfo.txt", "r").read()

print("Searching for average periods")
thread_1_cores = df2["thread_1_core"]
thread_2_cores = df2["thread_2_core"]
r = 0
if len(thread_1_cores) != 0:
    for i in range(0, len(thread_1_cores)):
        if thread_1_cores[i] == cores[0] and thread_2_cores[i] == cores[1]:
            r = i
            print("Found r")
            break
period_1 = df2["period_1"][r]
period_2 = df2["period_2"][r]
freq_1 = 1/period_1
freq_2 = 1/period_2
print("period_1:", period_1)
print("period_2:", period_2)

name = f.split("\n")[13][33:]

data = df["thread_1_round_trip_nano"]
data2 = df["thread_2_round_trip_nano"]

data = data / 2
data2 = data2 / 2

data_lo = data.quantile(0.001, interpolation="lower")
data2_lo = data2.quantile(0.001, interpolation="lower")
data_hi = data.quantile(0.999, interpolation="higher")
data2_hi = data2.quantile(0.999, interpolation="higher")
xlim = [max(0, min(data_lo, data2_lo)-100), max(data_hi, data2_hi)]

print("data1 max: ", np.max(data))
print("data2 max:", np.max(data2))

test = [x for x in data2 if x > 500]
print("Len test:", len(test))

ax = sns.ecdfplot(data=data, color='blue', label="Thread 1 ECDF")
sns.ecdfplot(data=data2, color='red', label="Thread 2 ECDF")

ax.set_title(name + ": Cumulative Density plots, Cores: " + str(cores[0]) + ", " + str(cores[1]))
ax.set(xlabel="CPU Clock Cycles", ylabel="Cumulative Density")
ax.set_xlim(xlim)

# ax2 = ax.twinx()

# sns.ecdfplot(data=data, color='royalblue', label="Thread 1 ECDF")
# sns.ecdfplot(data=data2, color='lightcoral', label="Thread 2 ECDF")

# ax2.set(ylabel="Cumulative Density")

values = {}
for x in data:
    if x in values.keys():
        values[x] += 1
    else: 
        values[x] = 1

print(values)

print("Print sorted values")
print(sorted(values.items()))

ax.legend(loc=0)
ax.xaxis.set_minor_locator(AutoMinorLocator(10))
ax.yaxis.set_minor_locator(AutoMinorLocator(5))

plt.grid(b=True, which='major', color='darkgray', linewidth=1)
plt.grid(b=True, which='minor', color='lightgray', linestyle=":")

quantiles = data.quantile([0.5, 0.90, 0.99])
quantiles2 = data2.quantile([0.5, 0.90, 0.99])
mean = data.mean()
mean2 = data2.mean()

text = ""
text += f"\\textbf{{Thread 1 statistics:}}\n"
text += f"Avg. TSC freq.: {freq_1:.3f} GHz\n"
text += f"Avg. TSC cycle time: {period_1} ns\n"
text += f"Mean: {mean:.1f} cycles = {mean*period_1:.1f} ns\n"
text += f"q(0.50): {quantiles[0.5]} cycles = {quantiles[0.5]*period_1:.1f} ns\n"
text += f"q(0.90): {quantiles[0.90]} cycles = {quantiles[0.90]*period_1:.1f} ns\n"
text += f"q(0.99): {quantiles[0.99]} cycles = {quantiles[0.99]*period_1:.1f} ns\n"

text += f"\\textbf{{Thread 2 statistics:}}\n"
text += f"Avg. TSC freq.: {freq_2:.3f} GHz\n"
text += f"Avg. TSC cycle time: {period_2} ns\n"
text += f"Mean: {mean2:.1f} cycles = {mean2*period_2:.1f} ns\n"
text += f"q(0.50): {quantiles2[0.5]} cycles = {quantiles2[0.5]*period_2:.1f} ns\n"
text += f"q(0.90): {quantiles2[0.90]} cycles = {quantiles2[0.90]*period_2:.1f} ns\n"
text += f"q(0.99): {quantiles2[0.99]} cycles = {quantiles2[0.99]*period_2:.1f} ns"

anchored_text = AnchoredText(text, loc=4)
ax.add_artist(anchored_text)

plt.show()