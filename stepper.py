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

    def __init__(self, accel, max_speed, micro_delay):
        """
        All parameters are dependent on the motor, system etc.

        :param accel: acceleration in steps/s²
        :param max_speed: max speed in steps/s
        :param micro_delay: is used as threshold for changing to micro stepping, when above this delay we will micro
                            step
        """

        # Absolute position (full steps)
        self.pos = 0

        # The position we want to move to.
        self.target_pos = 0

        # Steps used for acceleration, to know when we need to decelerate.
        self.accel_steps = 0

        # 1 or -1
        self.dir = 1

        # This is essentialy max speed. Delay will be moved to nearest
        # point above this. We want delay to be predictable when decelerating. This changes with shift and micro.
        self.min_delay = int(1/max_speed * 1e6)

        # Delay 0 will only be used at start and end. Delay changes with shift and micro.
        self.delay0 = self.delay = int(math.sqrt(1/accel) * 1e6)

        # Level of micro stepping right now.
        self.micro_delay = micro_delay
        self.micro_up_treshold = micro_delay
        self.micro_down_treshold = micro_delay >> 1
        self.micro = 0

        # Delays will be shifted to utilize as many bits in uint32_t as possible. Shift will only be done when
        # max_speed, acceleration/deceleration changes. The number of shifted bits will be stored here.
        self.shift_treshold = 1 << 30
        self.shift = min(self._possible_shift(self.min_delay), self._possible_shift(self.delay0),
                         self._possible_shift(micro_delay))
        self.delay0 <<= self.shift
        self.delay <<= self.shift
        self.min_delay <<= self.shift
        self.micro_delay <<= self.shift
        self.micro_up_treshold = self.micro_delay
        self.micro_down_treshold = self.micro_delay >> 1

    def _possible_shift(self, value):
        """ Return possible shift given the current shift_threshold. """
        shift = 0
        while value < self.shift_treshold:
            shift += 1
            value <<= 1
        return shift

    def step(self):
        """ Returns next delay based on speed and target pos. Algorithm now, delays later. """

        aligned = self.pos & (-1 << self.micro) == self.pos

        # Change micro stepping level, only changed micro step mode when aligned. TODO Verify that this does not
        # destroy precision shifted delay. But do that when algorithm is done.
        if aligned:
            while self.delay > self.micro_delay and self.micro < 5 and False:
                self.micro += 1
                self.delay >>= 1
                self.min_delay >>= 1
                self.accel_steps <<=1
                self.pos <<= 1
                self.target_pos <<=1

            while self.delay < self.micro_delay >> 1 and  self.micro > 0:
                self.micro -= 1
                self.delay <<= 1
                self.min_delay <<= 1
                self.accel_steps >>= 1
                self.pos >>= 1
                self.target_pos >>= 1

            # Set mode here.

        distance = self.target_pos - self.pos

        # Handle stopped state.

        if aligned and self.accel_steps <= 1:
            # It is possible to stop now if we want to, no need to decelerate.

            if self.pos == self.target_pos:
                # We have arrived and can stop.
                self.accel_steps = 0
                self.delay = self.delay0 >> self.micro
                return 0

            if (self.dir > 0) == (distance < 0):
                # Change dir, allow some time for it.
                self.accel_steps = 0
                self.dir = -self.dir
                return self.min_delay >> self.shift

        # TODO Step here and calculate delay later to be able to include time consuming calculation in next delay.

        # Should we accelerate?

        # What if distance = 2 and we accel to a place where decel is
        # impossible? But we also need to be able move 1 step.
        if self.dir * distance > 0 and self.dir * distance > self.accel_steps and self.delay >= self.min_delay:
            if self.accel_steps == 0:
                delta = 0  # Set to delay0 here instead, but then we need to handle shifted state.
            else:
                delta = self.delay * 2 // (4 * self.accel_steps + 1)
            self.accel_steps += 1
            self.delay -= delta

        # Should we decelerate?

        # What if min_delay changed? How do we know that we need to
        # decelerate no new max speed? last delay would do it.
        elif self.dir * distance <= self.accel_steps:
            if self.accel_steps <= 1:
                self.accel_steps = 0
                self.delay = self.delay0 >> self.micro
            else:
                self.accel_steps -= 1
                self.delay += self.delay * 2 // (4 * self.accel_steps - 1)

        # Handle ongoing micro stepping (implement later).
        self.pos += self.dir

        # Return delay
        return max(self.delay, self.min_delay) >> self.shift

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


