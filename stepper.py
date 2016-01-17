# -*- coding: utf-8 -*-

#
# Pseudo code for writing stepping acceleration code for microcontroller.
#

import math


class Stepper(object):
    """
    Stepper class to later convert to c++.

    * Can change target_pos at any time.

    * Will micro step below certain speed.

    * Updates to acceleration/dir will be done at full steps only.

    * All times are in us.

    * Target pos outisde max and min will be changed to a pos within max and min.

    * Max speed can be changed at any time (but decelration will take it's time).

    * Acceleration can only be changed when speed is 0.

    """

    def shift_down(self):
        """ Downshift to 0 then shift up to max depending on the values that should be shifted. """
        for i in range(len(self.delay0)):
            self.delay0[i] >>= self.shift
        self.delay >>= self.shift
        self.target_delay >>= self.shift
        self.smooth_delay >>= self.shift
        self.shift = 0

    def shift_up(self):
        d_max = max(self.delay0[-1], self.target_delay, self.smooth_delay) << self.shift
        while d_max < self.shift_treshold:
            self.shift += 1
            d_max <<= 1

        for i in range(len(self.delay0)):
            self.delay0[i] <<= self.shift
        self.delay <<= self.shift
        self.target_delay <<= self.shift
        self.smooth_delay <<= self.shift

    def set_target_speed(self, target_speed):
        """ Set target speed which controls target_delay (min delay). """
        self.shift_down()
        self.target_delay = int(1 / target_speed * 1e6)
        if self.target_delay > self.delay:
            self.state = self.DECEL
        else:
            self.state = self.ACCEL
        self.shift_up()

    ACCEL, DECEL, TARGET_SPEED = range(3)

    def __init__(self, accel, target_speed, smooth_dealy):
        """
        All parameters are dependent on the motor, system etc.

        :param accel: acceleration in steps/s²
        :param target_speed: max speed or the speed we want to run in, unit is steps/s
        :param smooth_dealy: is used as threshold for changing to micro stepping, when above this delay we will micro
                              step, this should be choosen as a delay when the motor runs smooth
        """

        self.target_delay = 0

        # Delays will be shifted to utilize as many bits in uint32_t as possible. Shift will only be done when
        # max_speed, acceleration/deceleration changes. The number of shifted bits will be stored here.
        self.shift_treshold = 1 << 30
        self.shift = 0

        # This is driver dependent.
        self.micro_levels = 6

        # Absolute position (full steps)
        self.pos = 0

        # Steps used for acceleration, to know when we need to decelerate.
        self.accel_steps = 0

        # 1 or -1
        self.dir = 1

        # The position we want to move to.
        self.target_pos = 0

        # Delay 0 will only be used at start and end. Delay changes with shift and micro. When micro shifting micro 1
        # step delay0 should be divided by sqrt(2) but then I don't shift with micro so that means it should be
        # multiplied by sqrt(2)/2 which is 1/sqrt(2), let's make an array out of that.
        d0 = int(math.sqrt(1/accel) * 0.676 * 1e6)
        self.delay0 = [int(d0 * math.sqrt(1 << m)) for m in range(self.micro_levels)]
        self.delay = self.delay0[0]
        print(self.delay0)

        # Level of micro stepping right now. Setting of start micro is flawed since delay0 is dependent on micro and
        # micro is dependent on delay0. Setting to 0 at start since that is simplest.
        self.micro = 0
        self.smooth_delay = smooth_dealy

        self.set_target_speed(target_speed)

        self.state = self.ACCEL

    def step(self):
        """ Returns next delay based on speed and target pos. Algorithm now, delays later. """

        aligned = self.pos & (-1 << self.micro) == self.pos

        # Change micro stepping level, only changed micro step mode when aligned. Delays will not be shifted since
        # it is better to pretend that they are shifted to keep precision.
        if aligned:
            # Only distances are changed with micro change. Delays should be halved for each micro level, but this is
            # implicit and is done on return. In turn d0 needs to be calculated with this in mind. If setting
            # microed d0 it should be d0 * sqrt(2)^micro_level. The target_delay is based on micro level 0 and since
            # delay is not micro shifted until return target_delay doesn't need to be micro shifted either.

            delay = max(self.delay, self.target_delay)

            while self.micro > 0 and delay < self.smooth_delay << self.micro:
                self.micro -= 1
                self.accel_steps >>= 1
                self.pos >>= 1
                self.target_pos >>= 1

            while self.micro < 5 and delay > self.smooth_delay << self.micro:
                self.micro += 1
                self.accel_steps <<= 1
                self.pos <<= 1
                self.target_pos <<= 1

            # Set mode here.

        distance = self.target_pos - self.pos

        # Handle non stepping states (stopped).

        if aligned and self.accel_steps <= 1:
            # It is possible to stop now if we want to, no need to decelerate more.

            if distance == 0:
                # We have arrived, so stop.
                self.accel_steps = 0
                self.delay = self.delay0[self.micro]
                self.state = self.ACCEL
                return 0

            if (self.dir > 0) == (distance < 0):
                # Change dir, allow some time for it.
                self.accel_steps = 0
                self.dir = -self.dir
                self.state = self.ACCEL
                return self.target_delay >> self.shift

        # TODO Step here and calculate delay later to be able to include time consuming calculation in next delay.
        # The delay is the waiting needed for this step to mechanically reach its target, when waiting is done the
        # step is done.

        self.pos += self.dir

        # Stepping state changes, most important rule first.

        if self.dir * distance <= self.accel_steps:
            # We are going in the wrong direction. Or we need to break now or we will overshoot.
            self.state = self.DECEL

        elif self.state == self.ACCEL and self.delay < self.target_delay:
            # We have reached a good speed
            self.state = self.TARGET_SPEED

        elif self.state == self.DECEL and self.delay >= self.target_delay:
            # Speed changed before but speed is good now.
            self.state = self.TARGET_SPEED

        # Do it

        if self.state == self.DECEL:
            if self.accel_steps <= 1:
                self.accel_steps = 0
                self.delay = self.delay0[self.micro]
            else:
                self.accel_steps -= 1
                self.delay += self.delay * 2 // (4 * self.accel_steps - 1)
            return self.delay >> self.micro >> self.shift

        if self.state == self.ACCEL:
            if self.accel_steps == 0:
                delta = 0
                self.delay = self.delay0[self.micro]
            else:
                delta = self.delay * 2 // (4 * self.accel_steps + 1)
            self.accel_steps += 1
            self.delay -= delta

        return max(self.delay, self.target_delay) >> self.micro >> self.shift

    # Accelerate
    #
    # dn+1 = dn ( 1 + 4 * s - 2) / (4 * s + 1)
    # dn+1 = dn ( 1 - 2 / (4 * s + 1))
    # dn+1 = dn - dn * 2 / (4 * s + 1))

    # Decelerate
    #
    # dn-1 = dn * (4n + 1) / (4n - 1)
    # dn-1 = dn * (4n - 1 + 2) / (4n - 1)
    # dn-1 = dn * 2 // (4n - 1)


