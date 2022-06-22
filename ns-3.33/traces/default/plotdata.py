'''
Author: Zhang Bochun
Date: 2022-06-16 22:45:12
LastEditTime: 2022-06-22 21:01:12
LastEditors: Zhang Bochun
Description: 
FilePath: /ns-3.33/traces/default/plotdata.py
'''
#!/usr/bin/python

import copy
import numpy as np
import os
import pandas as pd
import matplotlib.pyplot as plt


def plot(filename):
    data = pd.read_csv(filename, sep='\t', engine='python', header=None).values
    print(type(data))

    data_time = (data[0:, 0]).tolist()
    data_value = (data[0:, 1]).tolist()

    ret_time, ret_value = [], []
    for index_ in range(len(data_time)):
        if 0000 < data_time[index_] < 1000.0 and data_value[index_]:
            ret_time.append(data_time[index_])
            ret_value.append(data_value[index_])

    return ret_time, ret_value



path = os.getcwd() + "/traces/default"
path1 = path + "/cubic_simulation/"
temp = os.listdir(path1)
folds = [fold for fold in temp if "2flow - scheme- RTT = 100ms - 0.05 -0.20" in fold]
print(folds)

ax_list = [{}]
fig_list = [{}]

color_list = ['#9e5fe2', '#f6c085', '#89b883', '#1E90FF', "red", "grey"]
# value_names = ["rtt", "cwnd", "inflight", "sendrate", 'receiverate', 'delivered']
# value_names = ['rtt', 'cwnd', 'receiverate']
# value_names = ['cwnd']
line_styles = ['solid', 'solid', 'solid', ':', '-.', '-.', '-.', '-.']
# value_names = ["sendrate", 'receiverate', "delivered"]
# value_names = ["receiverate", "sender_cwnd", "receiver_rtt"]
value_names = ["rate_plot", "sender_cwnd", "receiver_rtt"]
#
# value_names = ['receiverate']
# value_names = ["datarate", "maxrate"]

for i, fold in zip(range(len(folds)), folds):                      # 1 flow, 2 flow, 4 flow
    files = os.listdir(path1 + fold)

    for j, value_name in zip(range(len(value_names)), value_names):        # rtt, cwnd, inflight, sendrate, receiverate
        file_filters = [file for file in files if value_name in file]
        file_filters.sort()
        print(file_filters)

        fig, ax = plt.subplots(figsize=(8, 4), dpi=300)
        fig_list.append({value_name: fig})
        # label_list = []

        flow = len(file_filters)
        for k, file_filter in zip(range(flow), file_filters):
            if value_name == 'recerate' or value_name == 'rate_plot':
                splits = file_filter.split('_')
                label = splits[0]
                time, value = plot(path1 + fold + r'/' + file_filter)
                ax.plot(time, value, color=color_list[k % 6], linestyle=line_styles[k % 8], label=label)

            else:
                splits = file_filter.split('_')

                sour_ip, sour_port = splits[0], splits[1]
                dest_ip, dest_port = splits[3], splits[4]
                side = splits[6]
                value = ''
                for index in range(7, len(splits)):
                    value = value + splits[index] + " "
                value = value.split('.')[0]
                print("sour ip:", splits[0], ",", "sour port:", splits[1], ",",
                    "dest ip:", splits[3], ",", "dest port:", splits[4], ",", "value:", value)

                label = sour_ip + ':' + sour_port + ' to ' + dest_ip + ':' + dest_port + ' - at ' + side + ' - ' + value

                time, value = plot(path1 + fold + r'/' + file_filter)
                ax.plot(time, value, color=color_list[k % 6], linestyle=line_styles[k % 8], label=label)

        title = 'cubic - ' + fold
        ax.set(axisbelow=True)
        ax.set_xlabel(xlabel='time(ms)', loc='center',
                      fontdict={'family': 'Times New Roman', 'size': 'x-large', 'style': 'italic'})
        ax.set_ylabel(ylabel=value_name, loc='center',
                      fontdict={'family': 'Times New Roman', 'size': 'x-large', 'style': 'italic'})
        ax.set_title(title, pad=8.0, fontdict={'family': 'Times New Roman', 'size': 'x-large'})
        plt.sca(ax)
        plt.xticks(fontproperties='Times New Roman')
        plt.yticks(fontproperties='Times New Roman')
        ax.legend(loc='best', frameon=True, ncol=1, prop={'family': 'Times New Roman', 'size': 10})

        save_path = path + "/picture/" + fold
        if not os.path.exists(save_path):
            os.makedirs(save_path)
        plt.savefig(save_path + "/" + value_name)

if __name__ == "__main__":
    # plt.show()
    print("end")
