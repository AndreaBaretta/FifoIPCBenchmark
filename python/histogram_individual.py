import numpy as np
import pandas as pd
import os
import matplotlib.pyplot as plt
import seaborn as sns
import scipy

dir_path = os.path.dirname(os.path.realpath(__file__))

df = pd.read_csv(dir_path + "/../builds/release/data/FifoIpcLatency_0_1.csv")

data = df["thread_2_round_trip_nano"] # generate samples from normal distribution (discrete data)

print(np.max(data))

# data = [x for x in data if x < 500]
# norm_cdf = scipy.stats.norm.cdf(x) # calculate the cdf - also discrete
# plt.hist(data, color = 'blue', edgecolor = 'black',
        #  bins = int(180/5))
# plt.figure(figsize=(16,8))

# seaborn histogram
sns.distplot(data, hist=True, kde=True, 
             bins=int(80), color = 'blue',
             hist_kws={'edgecolor':'black'}, kde_kws={"bw":0.5, "gridsize":1000})
# Add labels
plt.show()
# def myHist(data):
#     plt.hist(data[100:], bins=len(data), density=True, cumulative=True, histtype="step")
#     plt.title('Cumulative step histograms')
#     plt.xlabel('Latency (ns)')
#     plt.ylabel('Likelihood of occurrence')
#     plt.show()

# def hist(data, bins, title, labels, range = None):
#     fig = plt.figure(figsize=(15, 8))
#     ax = plt.axes()
#     values, base, _ = plt.hist( data  , bins = bins, alpha = 0.5, color = "green", range = range, label = "Histogram")
#     ax_bis = ax.twinx()
#     values = np.append(values,0)
#     ax_bis.plot( base, np.cumsum(values)/ np.cumsum(values)[-1], color='darkorange', marker='o', linestyle='-', markersize = 1, label = "Cumulative Histogram" )
#     plt.xlabel(labels)
#     plt.ylabel("Proportion")
#     plt.title(title)
#     ax_bis.legend()
#     ax.legend()
#     plt.grid(True)
#     plt.show()
#     return
    
# values, base = np.histogram(data, bins=40)
# cumulative = np.cumsum(values)
# plt.plot(base[:-1], cumulative, c='blue')
# plt.plot(base[:-1], len(data)-cumulative, c='green')

# plot the cdf
# sns.lineplot(x=x, y=norm_cdf)

# myHist(data)
hist(data, 40, "Cool Hostogram", "Latency")