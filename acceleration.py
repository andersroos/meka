#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import math
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

from stepper import Stepper

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


def plot(x, y, *dfs):
    """ Plot x and y axis of dfs in common graph. """
    ax = None
    for df in dfs:
        ax = df[[x, y]].set_index(x).plot(kind='line', ylim=(0, None), xlim=(0, None), ax=ax)

    
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
    # Har nån knasknäcki början

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


def accel_2_micro(steps, a):
    df = pd.DataFrame(index=np.arange(0, steps * 16), columns=('v', 's', 'd', 't'))

    t = 0.0
    d0 = d = math.sqrt(1/a)

    p = 0.0
    delta = 1.0
    s = 1
    for i in range(steps * 8):
        s += 1
        p += delta
        d -= d * 2 / (4 * s + 1)
        t += d
        df.loc[s] = [delta/d, p, d, t]
        if s == 10:
            delta /= 16
            s *= 16
            d /= 16

    return df.dropna()


def accel_2_integer(steps, a):

    stepper = Stepper(a, 1e4, 1000)
    stepper.target_pos = steps

    df = pd.DataFrame(index=np.arange(0, steps * 16), columns=('v', 's', 'd', 't', 'adj_d', 'micro'))

    t = 0
    s = 0
    while True:
        d = stepper.step()
        if d == 0:
            break

        m = 1 << stepper.micro_level
        df.loc[s] = [1e6/d/m, stepper.pos / m, d, t/1e6, d // m, m]
        t += d
        s += 1
    return df.dropna()

def move_a_bit(a):
    stepper = Stepper(a, 1e4, 1000)

    df = pd.DataFrame(index=np.arange(0, 1e4), columns=('v', 's', 'd', 't', 'p'))

    t = 0
    s = 0
    try:
        stepper.target_pos = 1500
        for i in range(200):
            d = stepper.step()
            # print(stepper.pos, stepper.target_pos, d)
            if d == 0:
                break
            df.loc[s] = [1e6/d, s, d/1e6, t/1e6, stepper.pos]
            t += d
            s += 1
        stepper.target_pos = 150
        for i in range(1500):
            d = stepper.step()
            # print(stepper.pos, stepper.target_pos, d)
            if d == 0:
                break
            df.loc[s] = [1e6/d, s, d/1e6, t/1e6, stepper.pos]
            t += d
            s += 1
    except:
        print("FAIL")
    return df.dropna()

a = 20000.0 # steps / s2
s = 1500
# df0 = accel_0(s, a)
# df1 = accel_1(s, a)
# df2 = accel_2(s, a)
dfi = accel_2_integer(s, a)
# dfu = accel_2_micro(s, a)
# dfm = move_a_bit(a)

# print("df0\n", df0.head())
print("dfi\n", dfi)
# print("dfu\n", dfu)
# print("dfm\n", dfm)

plot('t', 's', dfi)
plot('t', 'd', dfi)
plot('t', 'v', dfi)
plot('s', 'v', dfi)
plt.show()

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
    
