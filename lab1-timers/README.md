Lab1 Experiments & Results
==========================

Part 1: WCET Analysis
---------------------

### Task

Using WCET static or dynamic analysis, determine the number of iterations
required in a for-loop to occupy the CPU for 10ms. Use this loop to blink the
red LED at 1HZ (i.e. a period of 1000ms). 

### Results

Using a few commands, one can generate a listing of the assembly for the
progra  m along with related C source lines.  See the Makefile for the method
I used:
  
    $ make lab1.lst

This generates a listing with the following for a function where I call
WAIT_10ms (NOTE: compiled with -O0 which eliminates any optimizations).  The
number of instructions decreases significantly (to the point of removing the
reference to __ii as it is unused if optimizations are enabled).  I have
identified the repeated instructions with asterix below.

    static void
    delay_10ms(void)
    {
         18e:       a0 e0           ldi     r26, 0x00       ; 0
         190:       b0 e0           ldi     r27, 0x00       ; 0
         192:       ed ec           ldi     r30, 0xCD       ; 205
         194:       f0 e0           ldi     r31, 0x00       ; 0
         196:       0c 94 9d 13     jmp     0x273a  ; 0x273a <__prologue_saves__+0x20>
        WAIT_10MS
    // Loop Setup (Preamble)
         19a:       10 92 ab 05     sts     0x05AB, r1
         19e:       10 92 ac 05     sts     0x05AC, r1
         1a2:       10 92 ad 05     sts     0x05AD, r1
         1a6:       10 92 ae 05     sts     0x05AE, r1
         1aa:       13 c0           rjmp    .+38            ; 0x1d2 <delay_10ms+0x44>
    //******************************************************************************
    // Next Iteration Actions (e.g. __ii++, almost every time)
         1ac:       80 91 ab 05     lds     r24, 0x05AB
         1b0:       90 91 ac 05     lds     r25, 0x05AC
         1b4:       a0 91 ad 05     lds     r26, 0x05AD
         1b8:       b0 91 ae 05     lds     r27, 0x05AE
         1bc:       01 96           adiw    r24, 0x01       ; 1
         1be:       a1 1d           adc     r26, r1
         1c0:       b1 1d           adc     r27, r1
         1c2:       80 93 ab 05     sts     0x05AB, r24
         1c6:       90 93 ac 05     sts     0x05AC, r25
         1ca:       a0 93 ad 05     sts     0x05AD, r26
         1ce:       b0 93 ae 05     sts     0x05AE, r27
    // Loop Termination Check (Executed Every Time)
         1d2:       80 91 ab 05     lds     r24, 0x05AB
         1d6:       90 91 ac 05     lds     r25, 0x05AC
         1da:       a0 91 ad 05     lds     r26, 0x05AD
         1de:       b0 91 ae 05     lds     r27, 0x05AE
         1e2:       8d 38           cpi     r24, 0x8D       ; 141
         1e4:       20 e2           ldi     r18, 0x20       ; 32
         1e6:       92 07           cpc     r25, r18
         1e8:       20 e0           ldi     r18, 0x00       ; 0
         1ea:       a2 07           cpc     r26, r18
         1ec:       20 e0           ldi     r18, 0x00       ; 0
         1ee:       b2 07           cpc     r27, r18
         1f0:       e8 f2           brcs    .-70            ; 0x1ac <delay_10ms+0x1e>
    //******************************************************************************
    }
         1f2:       20 96           adiw    r28, 0x00       ; 0
         1f4:       e2 e0           ldi     r30, 0x02       ; 2
         1f6:       0c 94 b9 13     jmp     0x2772  ; 0x2772 <__epilogue_restores__+0x20>
  
From this, we can use the datasheet in order to determine how long it will
take to execute N iterations of the WAIT_10ms loop.  Here's the work behind
that function:
  
    body = (8 * lds + 1 * adiw + 2 * adc + 4 * sts + 1 * cpi + 3 * ldi +
            3 * cpc + 1 * brcs)

The general form of the computation being:
  
    cycles = N * body
    time(s) = cycles / 20000000

If we plug in the values for the data sheet for each instruction (done so
here with python), we get the following:

    >>> lds = 2
    >>> adiw = 2
    >>> adc = 1
    >>> sts = 2
    >>> cpi = 1
    >>> ldi = 1
    >>> cpc = 1
    >>> brcs = 2  # 1/2, so WCET is 2
    >>> body = (8 * lds + 1 * adiw + 2 * adc + 4 * sts + 1 * cpi + 3 * ldi +
    ...         3 * cpc + 1 * brcs)
    >>> body
    37

Let's ignore the preamble instructions as noise, giving us:

    time(s) =  37 * N / 20000000
    N = (20000000 * time(s)) / 37

Solving for 10ms, we get the following:

    N = (20000000 * 0.01) / 37
      = 5405.4

So to wait 10ms we must execute the for loop ~5405 times.  Using this
code empirically delivers good results in a snippet like this:

    LED_ENABLE(RED);
    int i = 0, j = 0;
    for (j = 0; j < 10; j++) {
        /* wait 1 s */
        for (i = 0; i < 100; i++) {
            WAIT_10MS;
        }
        LED_TOGGLE(RED);
    }

Part 2: 1ms Timer
-----------------

### Task

Create a software timer (8-bit) with 1ms resolution, then blink the red LED
inside a cyclic executive at a user-specified rate using your software timer.
Essentially, the ISR is releasing the red LED task.  The period for the
release of the red led toggle can be configured via the menu system.

### Results

This is done in timers.c using the generic timer setup code that I wrote.
The ISR does two things:
 1. Sets a flag that indicates that the red led toggle "task" can be
    released.
 2. Let's the generic (but simple) scheduler I wrote schedule other tasks
    to execute.  The red led release could be easily written as a task
    that the scheduler calls every N milliseconds.

Part 3: Yellow LED (from ISR)
-----------------------------

### Task

Create another software timer (timer3) with 100ms resolution (10Hz), and blink
the yellow LED inside the ISR for the timer interrupt. In this case, the
system is being polled at a specific frequency to determine the readiness of
a task.

### Results

This timer (TC3) is setup by the generic timer setup code I wrote in timers.c
and the LED is toggled from the ISR.  I don't really understand what the last
sentence from the assignment is trying to say (seems unrelated).  There is
no polling to change this LED as we do it directly from the ISR.  The
period for the yellow led can be configured at runtime via the menu system;
the generic timer setup logic was particularly beneficial for this feature.

Part 4: Green LED (PWM Pulse)
-----------------------------

### Task

Create a Compare Match Interrupt with a frequency equal to the user-specified
frequency for blinking the green LED (use timer1). Generate a PWM pulse on OC1A
(aka Port D, pin 5) to toggle green.

### Results

The setup for this is done in timers.c with the period for the timer being
setup by the generic timer setup code with a couple extra lines to setup
the toggle on compare match.  The period for toggling the green LED can
be configured at runtime via the menuing system.

Experiment 1
------------

### Task

Use your original version of toggling the red LED that uses for-loops. Toggle
all 3 at 1Hz. (Do not type in any menu options while you are toggling until the
1 minute is up). How good was your WCET analysis of the for loop? If it is very
far off, adjust it. Why did I not want you to use the menu while running the
experiment? 

### Results

The results showed that the red led seems to blink at a slightly slower rate
than the others.  I believe there are a few reasons for this:
 1. There are additional instructions outside the core of the for loop that
    are coming into play in this calculation.  This includes the preamble
    executed each time WAIT_10MS is executed as well as the overhead for
    the outer loops.
 2. There are interrupts firing that are taking up CPU time.  Notably, the
    interrupts for the green/yellow leds.

As it turns out, using the menu would not actuall affect the calculations
at all on this board as receiving input is polled rather than triggered
by interrupts.  If we did process incoming data in interrupts, this would
have further messed up the calculations.  Because I am doing some extra
work in the 1ms interrupt for scheduling (I still have the 1ms interrupt
enabled in this test), I likely have some extra overhead when compared with
others solutions.  The tasks aren't released in the ISR, but we do walk
through the task list to release tasks.

Experiment 2
------------

### Task

Use your software timer to toggle the red LED. Toggle all 3 at 1Hz. Simply
observe the final toggle count. All should be about 60 (maybe the red is off
by 1). If this is not the case, you probably set something up wrong, and you
should fix it. 

### Results

Using my scheduler, I set up a task to log once every 60s.  At 60s increments
I saw the following counts (note that the 60s is based off the red timer, so
that match up doesn't necessarily imply that timer is more accurate):

    red toggles:    60
    yellow toggles: 61
    green toggles:  61
    ---
    red toggles:    120
    yellow toggles: 122
    green toggles:  122
    ---
    red toggles:    180
    yellow toggles: 183
    green toggles:  183
    ---
    red toggles:    239
    yellow toggles: 243
    green toggles:  243
    ---
    red toggles:    300
    yellow toggles: 304
    green toggles:  304
    ---
    red toggles:    360
    yellow toggles: 365
    green toggles:  365

This is close, but needs more investigation to figure out what the root
cause might be.

Experiment 3
------------

### Task

Set all LEDs to toggle at 2Hz (500ms). Place a 90ms busy-wait for-loop into the
ISR for the green LED. Toggle for 1 minute and record results. Now place a 90ms
busy-wait for-loop into the ISR for the yellow LED. Toggle for 1 minute and
record results. What did you observe? Did the busy-wait disrupt any of the
LEDs? Explain your results.

### Results

Green Test:
I saw all three lights blinking, with the red led blinking noticeably slower.
Interestingly, at 60s (measured according to ms interrupt), the following
very curious results were provided:

    red toggles:    120
    yellow toggles: 147
    green toggles:  147

In this test, there was a brief but noticeable delay between the toggle of the
green (first) and yellow LEDs.  The red LED was not in sync.

Yellow Test:
This test was similar to the last one with the red led being noticeably laggy
compared with the other two.  The main difference is that the green/yellow
LEDs did appear to stay in sync (no little delay between them):

    red toggles:    120
    yellow toggles: 147
    green toggles:  147

Experiment 4
------------

### Task

Repeat #3, except use a 110ms busy-wait. You probably won’t be able to use the
menu functions. If not, report that, and discuss what you observed from the
blinking. 

### Results

Results were pretty similar to the previous experiment but exhausted a bit
more as shown by the test results shown below.

Green Test:

    red toggles:    120
    yellow toggles: 154
    green toggles:  154

Yellow Test:

    red toggles:    119
    yellow toggles: 154
    green toggles:  154

Experiment 5
------------

### Task

Repeat #3, except use a 510ms busy-wait.

### Results

In both cases, the green led seemed to blink as expected and the red
led did not blink at all.  

Green Test:
The green LED blinked at what appeared to be the correct period.  The
yellow LED stayed solid on and the red LED did not appear to blink at
all.

Yellow Test:
In the yellow test, both the yellow and green LEDs blink, though the
Yellow LED appeared to lag from the green led over time.  Again, the RED
led did not 
    
Experiment 6
------------

### Task

Repeat #5 (i.e. 2Hz toggle with 510ms busy-wait), except place an sei() at
the top of the ISR with the for-loop in it.

### Results

Yellow Test:
In the yellow test with interrupts enabled, the red led stayed off but
the red and yellow leds blinked in sync.

Green Test:
Enabling interrupts in the green LED ISR did not seem to result in
any differences from the previous experiment.
