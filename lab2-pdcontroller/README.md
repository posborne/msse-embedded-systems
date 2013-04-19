Lab Assignment 2: PD Motor Control
----------------------------------

Assignment Description
======================

### Introduction

For this lab, you will implement a motor controller that can reach any
referenced motor position without significant oscillation using PD
control. Your goal is to find appropriate values for the gains Kd and
Kp in the PD equation, and find an appropriate "step size" for the
interpolator, both of which are described below.

Your system will be composed of the following parts:

 - A PD controller
 - A linear trajectory interpolator
 - A user interface

### PD Controller

The PD controller maintains the position of the motor using both the
reference and measured position of the motor. It is of the highest
priority in the system, and it should run at a frequency of 1KHz. The
controller that you will implement calculates a desired torque
according to the equation

    T = Kp(Pr - Pm) - Kd*Vm
 
where

    T = Output motor signal (torque) 
    Pr = Desired motor position 
    Pm = Current motor position 
    Vm = Current motor velocity (computed based on finite differences) 
    Kp = Proportional gain 
    Kd = Derivative gain

T is a signal that can be used directly to control the motor, except
check that it is in range { -TOP, TOP }, and use absolute(T) and set
motor direction appropriately.

Pm and Vm can be computed using the encoder attached to the motor. The
encoder generates a pair of signals as the motor shaft rotates, which
are used as input signals into 2 general I/O port pins on the
Orangutan. Using the Orangutan libraries (the wheel encoder example)
or your own code, set up pin change interrupts on these port pins and
count the number of signal changes to track the position of the motor.

Pr is provided through the user interface or as part of a hard-coded
trajectory.

Kp and Kd are the terms that you define that determine how your system
behaves. You will need to determine these values experimentally. See
discussion below under trajectory interpolator to figure out where to
start with these values. You should always be generating motor
commands, regardless of whether the reference position is changing or
not. This means that at any time, if you move the wheel manually, then
the PD controller should bring it back to the current reference
position. In other words, even if the motor is where it should be, do
not stop sending commands to the motor, instead send it 0 (or whatever
torque value your controller produces).

### Motor

The motor is attached to the Motor2 port of the Orangutan. This
corresponds to OC2B for the PWM drive signal and PC6 as the
directional signal. You should write your own code to control the
motor, but you may use the Orangutan code as reference material. You
use the torque value generated from the controller to adjust the duty
cycle of the waveform, thus the speed of the motor. The motors we are
using are Solarbotics Gear Motor 2 motors, and the encoders are
Solarbotics Wheel Watcher Rotation Encoders. These items can be found
at the following websites: http://www.solarbotics.com/products/gm2/
http://www.solarbotics.com/products/gmww02/

### Trajectory Interpolator

For this lab, the interpolator mainly serves the purpose of managing
the reference (desired) position and feeding it to the PD controller
to execute a complete trajectory (e.g. rotate forward 360 deg, rotate
backwards 90 deg, rotate forward 10 deg). The trajectory interpolator
is more meaningful when there are multiple motors that need to arrive
at a given point at the same time.

One potential problem with the PD controller is that the error term
(Pr-Pm) can start arbitrarily large, yet is vey small when close to
the reference position. This makes it difficult to find values for Kp
and Kd that work well in all situations. To remedy this problem, you
will use the interpolator to bound the error term to a threshold of
your choosing. I will refer to this as your step size.

To get started with finding your Kp, Kd, and step size, think about
the equation:

    T = Kp(Pr - Pm) - Kd*Vm 

Your torque value needs to be in the range -TOP to TOP as you defined
it in setting up your PWM (Note: you don't feed negative values to
your motor, instead set the direction to reverse and give it a
positive torque value). Consider the point when the motor is first
starting to move (Vm is 0) and it is more than one step size away
(thus, Pr-Pm = step_size). If you want T to be close to TOP at this
point (although you might want to start at about TOP/2 to see how it
goes), then Kp is your only unknown and you can start with it
there. With this value, it is likely that your motor will be
oscillating around its reference position (overshooting). The Kd term
helps with this, so start adding that in to see if you can get rid of
it (at least most of it). Test your gains with reference positions
both very close and very far from its current position.

### User Interface

The user interface consists of the following commands at a minimum
(feel free to add in whatever makes your life easier for programming
and debugging):

    L/l: Start/Stop Logging (print) the values of Pr, Pm, and T. 
    V/v: View the current values Kd, Kp, Vm, Pr, Pm, and T 
    R/r : Set the reference position to degrees 
    P: Increase Kp by an amount of your choice* 
    p: Decrease Kp by an amount of your choice 
    D: Increase Kd by an amount of your choice 
    d: Decrease Kd by an amount of your choice 
    
    * The amount to increase or decrease the gain by depends on how you set up the PWM channel

### Deliverables

Hand in all of the code necessary to compile your project. Please put
sufficient comments in your code so that I can follow what you are
doing. Also, if your code is not working in some aspect, please
include that in your report. In addition, submit a report that
addresses the following:

 1. Experiment with the speed of the motor: Run your motor at full
    speed. Modify your gains to achieve position control at that speed
    (as best you can). Slow the motor down as much as possible. Modify
    your gains to acheive position control. Repeat with one or two speeds
    in between. For each, record the approximate speed in rotations per
    second, record your equation (gains), and report on the behavior of
    the system and your ability to control the position.
 2. Change the step size to something very large (more than 2pi), and
    try a reference position of 4pi+current_position. How does system
    behavior differ from your tuned step size? Try tuning your
    controller for that very large step size. What happens if you then
    set the reference position to be very close to the current
    position (within a few degrees)?
 3. Using your optimally tuned values for the PD controller running at
    1kHz, graph Pm, Pr and T while executing the trajectory: rotate
    the motor forward 360 degrees, hold for .5 seconds, then rotate
    backwards for 360 degrees, hold for .5 seconds, rotate forwards for
    5 degrees. Be sure to graph the entire trajectory.
 4. Run your PD controller at 50Hz and 5Hz while graphing the same
    variables. Discuss the results.

Lab Report
==========

### Part 1: Motor Speed

> Experiment with the speed of the motor: Run your motor at full
> speed. Modify your gains to achieve position control at that speed
> (as best you can). Slow the motor down as much as possible. Modify
> your gains to achieve position control. Repeat with one or two speeds
> in between. For each, record the approximate speed in rotations per
> second, record your equation (gains), and report on the behavior of
> the system and your ability to control the position.

### Part 2: Step Size Modifications

> Change the step size to something very large (more than 2pi), and
> try a reference position of 4pi+current_position. How does system
> behavior differ from your tuned step size? Try tuning your
> controller for that very large step size. What happens if you then
> set the reference position to be very close to the current
> position (within a few degrees)?

### Part 3: Graphing of Pm, Pr, and T

> Using your optimally tuned values for the PD controller running at
> 1kHz, graph Pm, Pr and T while executing the trajectory: rotate
> the motor forward 360 degrees, hold for .5 seconds, then rotate
> backwards for 360 degrees, hold for .5 seconds, rotateforwards for
> 5 degrees. Be sure to graph the entire trajectory.

### Part 4: Graph Again at 50/5Hz

> Run your PD controller at 50Hz and 5Hz while graphing the same
> variables. Discuss the results.

