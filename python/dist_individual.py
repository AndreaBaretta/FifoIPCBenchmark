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

dir_path = os.path.dirname(os.path.realpath(__file__))

df = []
if len(sys.argv) == 1:
    print("Using default directory")
    df = pd.read_csv(dir_path + "/../builds/release/data/FifoIpcLatency_0_1.csv")
else:
    print("Used specified directory: " + sys.argv[1])
    df = pd.read_csv(sys.argv[1])

data = df["thread_1_round_trip_nano"]
data2 = df["thread_2_round_trip_nano"]


# data = data[1000:]
# data = [x for x in data if x < 500]
# data2 = [x for x in data2 if x < 500]

print("data1 max: ", np.max(data))
print("data2 max:", np.max(data2))

test = [x for x in data2 if x > 500]
print("Len test:", len(test))

sns.distplot(data, hist=False, kde=True, 
    bins=int(80), color = 'blue',
    hist_kws={'edgecolor':'black'}, kde_kws={"bw":0.5, "gridsize":1000})

sns.distplot(data2, hist=False, kde=True, 
    bins=int(80), color = 'red',
    hist_kws={'edgecolor':'black'}, kde_kws={"bw":0.5, "gridsize":1000})

values = {}
for x in data:
    if x in values.keys():
        values[x] += 1
    else: 
        values[x] = 1

print(values)

print("Print sorted values")
print(sorted(values.items()))

plt.show()