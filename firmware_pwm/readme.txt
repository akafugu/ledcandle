
Works very well with the akafugu LED candle board.


Notes:
======

The FUSE settings of the AVR are different to the original from akafugu.
Probably as a consequence of the increased clock-speed of the internal
RC-oscillator, the AVR won't start after programming when using 5V.
It will run fine with 3.3V or the lithium battery.

This issue can be addressed in two ways:

* switch your ISP programmer to 3.3V
* add a small bypassing capacitor right across pins #4 and #8

