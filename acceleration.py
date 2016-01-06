#!/usr/bin/env python

import math
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

def true_accel(steps, a):
    
    df = pd.DataFrame(index=np.arange(0, steps), columns=('v', 's', 'd', 't'))

    v = 0.0
    t = 0.0
    
    df.loc[0] = [0, 0, 0, 0];
    for s in np.arange(1, steps):
        v = math.sqrt(2 * a * s)
        t = t + 1/v
        df.loc[s] = [v, s, 1/v, t]
    return df.dropna()

df = true_accel(1500, 1000)

dft = df.set_index('t')
dft.plot(kind='line', subplots=True, layout=(1, 3), figsize=(18, 4), ylim=(0, None))
plt.show()

dfs = df.set_index('s')
dfs.plot(kind='line', subplots=True, layout=(1, 3), figsize=(18, 4), ylim=(0, None))
plt.show()

    
    
