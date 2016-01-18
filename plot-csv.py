#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import math
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

def plot(x, y, *dfs):
    """ Plot x and y axis of dfs in common graph. """
    ax = None
    for df in dfs:
        ax = df[[x, y]].set_index(x).plot(kind='line', ylim=(0, None), xlim=(0, None), ax=ax)

args = sys.argv[1:]

filename = args.pop(0)
print("reading csv file ", filename)

df = pd.read_csv(filename)

print(df)

while args:
    plot(args.pop(0), args.pop(0), df)

plt.show()
