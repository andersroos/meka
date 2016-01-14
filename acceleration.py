#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#
# Testing acceleration profiles for stepper.
#

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


def micro_1(steps, a):
    """ Normal unmicroed stepping. """
    df = pd.DataFrame(index=np.arange(0, steps * 16), columns=('v', 's', 'd', 't'))

    t = 0.0
    d0 = math.sqrt(1/a)
    s = 0        # steps = position
    while s < steps:
        if s == 0:
            d = d0 * 0.676
        else:
            d -= d * 2 / (4 * s + 1)
        s += 1
        t += d
        df.loc[s] = [1/d, s, d, t]

    return df.dropna()

def micro_8(steps, a):
    """ 8 microed stepping by faking distance twice as long. """
    df = pd.DataFrame(index=np.arange(0, steps * 16), columns=('v', 's', 'd', 't'))

    t = 0.0
    m = 8        # micro level
    d = d0 = math.sqrt(1/a/m)  # faster accel since distance is longer
    s = 0        # steps
    p = 0        # position
    p_d = 1/m    # position delta
    for s in range(800):
        if s == 0:
            d = d0 * 0.676
        else:
            d -= d * 2 / (4 * s + 1)
        s += 1
        p += p_d
        t += d
        df.loc[s] = [1/d/m, p, d, t]

    # m = 1
    # p_d = 1/m
    # d = d * 8
    # for s in range(100, 200):
    #     if s == 0:
    #         d = d0 * 0.676
    #     else:
    #         d -= d * 2 / (4 * s + 1)
    #     s += 1
    #     p += p_d
    #     t += d
    #     df.loc[s] = [1/d/m, p, d, t]

    return df.dropna()

def accel_2_integer(steps, a):

    stepper = Stepper(a, 10000, 700)
    stepper.target_pos = steps

    df = pd.DataFrame(index=np.arange(0, steps * 16), columns=('v', 's', 'd', 't', 'adj_d', 'micro', 'shift'))

    t = 0
    s = 0
    while True:
        d = stepper.step()
        if d == 0 or s > steps * 16:
            break

        m = 1 << stepper.micro
        v = 1e6 / d / m
        p = stepper.pos / m
        ad = d * m
        t += d
        df.loc[s] = [v, p, d, t / 1e6, ad, m, stepper.shift]
        s += 1
    return df.dropna()

def move_a_bit(a):
    stepper = Stepper(a, 2000, 1000)

    df = pd.DataFrame(index=np.arange(0, 1500 * 2), columns=('v', 's', 'd', 't'))

    t = 0.0
    s = 0
    try:
        stepper.target_pos = 1500
        while True:
            d = stepper.step()
            m = 1 << stepper.micro
            if d == 0 or stepper.pos / m >= 300:
                stepper.set_target_speed(6000)
                break
            t += d
            df.loc[s] = [1e6/d/m, stepper.pos/m, d/1e6, t/1e6]
            s += 1
        while True:
            d = stepper.step()
            m = 1 << stepper.micro
            if d == 0:
                break
            if 1e6/d/m < 200:
                i = 1
            df.loc[s] = [1e6/d/m, stepper.pos/m, d/1e6, t/1e6]
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
# print("df0\n", df0.head())
# print("dfu\n", dfu)

# m

dfm = move_a_bit(a)

print("dfm\n", dfm)
plot('t', 's', dfm)
plot('t', 'd', dfm)
plot('t', 'v', dfm)
plot('s', 'v', dfm)
plt.show()

# # i
#
# dfi = accel_2_integer(1500, a)
# print("dfi\n", dfi)
#
# plot('t', 's', dfi)
# plot('t', 'd', dfi)
# plot('t', 'v', dfi)
# plot('s', 'v', dfi)
plt.show()

# # micro
#
# m1 = micro_1(s, a)
# m8 = micro_8(s, a)
#
# print(m1.head(10))
# print(m8.head(40))
# plot('t', 'd', m1)
# plot('t', 'v', m1)
# plot('t', 'd', m8)
# plot('t', 'v', m8)
# plt.show()


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
    
