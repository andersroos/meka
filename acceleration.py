#!/usr/bin/env python3

import math
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

# Algorithm 0, velocity stopping distance formula:
#
# v = sqrt(2 * a * s)
#
# Algorithm 1, delay:
#
# d0 = sqrt(1/a) ???
# dn = d0(sqrt(n + 1) - sqrt(n))
#
# Algorithm 2, taylor approx delay:
#
# d0 = constant based on a
# dn = dn-1 * ( 1 - 2 / (4n + 1)) = dn-1 * (4n - 1) / (4n + 1)
# inverse is:
# dn-1 = dn * (4n + 1) / (4n - 1)


def plot_common(x, y, *dfs):
    """ Plot x and y axis of dfs in common graph. """

    ax = None
    for df in dfs:
        ax = df[[x, y]].set_index(x).plot(kind='line', ylim=(0, None), ax=ax)
    plt.show()

    
def accel_0(steps, a):
    
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
        if True or s < 500:
            d = d * (4 * s - 1) / (4 * s + 1)
        else:
            u = 1000 - s
            d = d * (4 * u + 1) / (4 * u -1 )
            if d < 0:
                break

    return df.dropna()

    
def accel_2_integer(steps, a):

    class Stepper(object):
        """ All times are in us. """

        def __init__(self, accel, max_speed):
            self.pos = 0
            self.accel_steps = 0
            self.delay0 = self.delay = int(math.sqrt(1/accel) * 1e6)
            self.target_pos = 0
            self.min_delay = int(1/max_speed * 1e6)

        @staticmethod
        def accelerate(d, s):
            factor = (4 * s - 1 ) / (4 * s + 1)
            rest_factor = - int(d) // (4 * s + 2)
            return d * factor

        @staticmethod
        def decelerate(d, s):
            factor = (4 * s + 1) / (4 * s - 1)
            rest_factor = int(delay0) // (4 * (s - 1) + 1)
            return rest_factor
            
        def step(self):
            """ Returns next delay based on speed and target pos. """
            if self.accel_steps > 0:
                self.delay = self.accelerate(self.delay, self.accel_steps)
            self.pos += 1
            self.accel_steps += 1
            return self.delay

    stepper = Stepper(a, 1e4)
    stepper.target_pos = steps
            
    df = pd.DataFrame(index=np.arange(0, steps), columns=('v', 's', 'd', 't'))

    t = 0.0
    df.loc[0] = [0, 0, 0, 0];
    for s in np.arange(1, steps):

        d = stepper.step() / 1e6
        t = t + d
        df.loc[s] = [1/d, s, d, t]
        
    return df
    
a = 10000.0 # steps / s2
    
df0 = accel_0(1500, a)
df1 = accel_1(1500, a)
df2 = accel_2(1500, a)
df3 = accel_2_integer(1500, a)

print(df2.head())
print(df3.head())

plot_common('t', 'd', df1, df2, df3)

# ax = df0[['t', 'd']].set_index('t').plot(kind='line', ylim=(0, None))
# df1[['t', 'd']].set_index('t').plot(kind='line', ylim=(0, None), ax=ax)
# df2[['t', 'd']].set_index('t').plot(kind='line', ylim=(0, None), ax=ax)
# df3[['t', 'd']].set_index('t').plot(kind='line', ylim=(0, None), ax=ax)
# plt.show()


# dft = df.set_index('t')
# dft.plot(kind='line', subplots=True, layout=(2, 3), figsize=(18, 4), ylim=(0, None))
# plt.show()
# 
# dfs = df.set_index('s')
# dfs.plot(kind='line', subplots=True, layout=(2, 3), figsize=(18, 4), ylim=(0, None))
# plt.show()

# Algo maste kunna prognostisera inbromsningar (target kan andras nar
# som helst), men jag tror man kan rakna fran speed faktiskt.
# Utfora decelaration pa samma satt som acceleration (alltid ligga pa samma kurva, ingen forandring vid max).
# Berakning vara tillrackligt snabb.
# Mikrostega.
# Bra kurva i start och slut.
    
