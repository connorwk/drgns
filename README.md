# drgns
An open source, embedded, high performance g-code-parser and CNC milling controller written in optimized C. Ported from grbl.

DRGNS stands for:
Delphino Replacement GRBL Now with Servos

This is a port of grbl to the TI C2000 Delphino line of DSPs.
I am developing this on the TI LAUNCHXL-F28379D Launchpad. A cheap development board for the DSP:
http://www.ti.com/tool/LAUNCHXL-F28379D

Current tasks are:
  1) To get the code working as intended initially.
     So driving steppers processing G-code over serial.
     A direct port.
  2) Implement a PID loop to allow for Servo motor drive, probably initially on CPU1's core as-well to quickly get things
     going as to not add complexity with interprocessor communications and interrupts
  3) Once this is working for 3 axis's on one core, then offload this code onto CPU2 for effecieny.
  4) Characterize performance, see where we can push the platform, if any limitations exist, where, can we get the
     CLAs involved to boost performace?
  5) Add support to grbl for more axis once we determine how many more we want to add.
  6) Add support to enable axis's as stepper OR servo! Initially we will focus on only servo drive but this would be an
     awesome feature. Though dealing with timing and how much performance is to be expected out of enabling/disabling
     axis's in each mode will need characterizing.
  7) ... Profit?

Note: Anything after Step 1 in the above list will probably be in flux, these are simply mind ramblings put down as a rough idea of a roadmap.


Disclaimer:
I am a hobbiest, this is the first time I am tackling a project of such a scale. I have a few friends who give code advice here and there
but this is a project all on my own. If you'd like to help with such a project, awesome! Please reach out to me!
I have no idea what I'm doing and so you should treat me as ignorant until you feel I understand something. :)
