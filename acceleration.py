#!/usr/bin/env python

import math
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Algorithm 1, delay:
#
# d0 = sqrt(1/a) ???
# dn = d0(sqrt(n + 1) - sqrt(n))
#
# Algorithm 2, taylor approx delay:
#
# d0 = constant based on a
# dn = dn-1 * ( 1 - 2 / (4n + 1)) = dn-1 * (4n - 1) / (4n + 1)
# inversern är:
# dn-1 = dn * (4n + 1) / (4n - 1)


def accel(steps, a):
    
    df = pd.DataFrame(index=np.arange(0, steps), columns=('v', 's', 'd', 't'))

    v = 0.0
    t = 0.0
    
    df.loc[0] = [0, 0, 0, 0];
    for s in np.arange(1, steps):
        v = math.sqrt(2 * a * s)
        t = t + 1/v
        df.loc[s] = [v, s, 1/v, t]
    return df.dropna()

def accel_1(steps, a):

    df = pd.DataFrame(index=np.arange(0, steps), columns=('v', 's', 'd', 't'))
    
    t = 0.0
    d0 = d = math.sqrt(1/a)
    
    df.loc[0] = [0, 0, 0, 0];
    for s in np.arange(1, steps):
        t = t + d
        df.loc[s] = [1/d, s, d, t]
        d = d0 * (math.sqrt(s + 1) - math.sqrt(s))
    return df.dropna()

def accel_2(steps, a):

    df = pd.DataFrame(index=np.arange(0, steps), columns=('v', 's', 'd', 't'))
    
    t = 0.0
    d0 = d = math.sqrt(1/a)
    
    df.loc[0] = [0, 0, 0, 0];
    for s in np.arange(1, steps):
        t = t + d
        df.loc[s] = [1/d, s, d, t]
        if s < 500:
            d = d * (1 - 2 / (4 * s + 1))
        else:
            u = 1000 - s
            d = d * (4 * u + 1) / (4 * u -1 )
            if d < 0:
                break

    return df.dropna()
    
a = 10000 # steps / s²
    
df0 = accel(1500, a)
df1 = accel_1(1500, a)
df2 = accel_2(1500, a)

ax = df0[['t', 'd']].set_index('t').plot(kind='line', ylim=(0, None))
df1[['t', 'd']].set_index('t').plot(kind='line', ylim=(0, None), ax=ax)
df2[['t', 'd']].set_index('t').plot(kind='line', ylim=(0, None), ax=ax)
plt.show()

# dft = df.set_index('t')
# dft.plot(kind='line', subplots=True, layout=(2, 3), figsize=(18, 4), ylim=(0, None))
# plt.show()
# 
# dfs = df.set_index('s')
# dfs.plot(kind='line', subplots=True, layout=(2, 3), figsize=(18, 4), ylim=(0, None))
# plt.show()

# Algo måste kunna prognostisera inbromsningar (target kan ändras när
# som helst), men jag tror man kan räkna från speed faktiskt.
# Utföra decelaration på samma sätt som acceleration (alltid ligga på samma kurva, ingen förändring vid max).
# Beräkning vara tillräckligt snabb.
# Mikrostega.
# Bra kurva i start och slut.
    
