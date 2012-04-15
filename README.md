LED Candle Firmware & CAD files
--------------------------

Firmware and CAD filesfor the [Akafugu Flickering LED Candle](http://www.akafugu.jp/posts/products/ledcandle/)

The Akafugu LED Candle is an artificial candle that imitates the flickering of
a real candle. Use it in place of a real tea candle: It will fit
inside a tea candle casing or any holder made for tea candles.

It comes as a kit of easy to solder components, and is suitable for beginners
to soldering.

FEATURES:

* Randomly flickering LED: Imitates a candle
* Fits inside a tea candle casing
* Open Source Firmware (available at [GitHub](https://github.com/akafugu/ledcandle))
* Open Source Hardware: Eagle PCB design files available at GitHub.
* On-board ISP header for upgrading firmware

Firmware
--------

The Akafugu LED Candle uses an ATTiny13A microcontroller. The firmware is written for the
avr-gcc compiler.

See [here](http://www.akafugu.jp/posts/resources/avr-gcc/) for avr-gcc installation instructions.

Plug in ISP programmer into the ISP header, and in the directory 'firmware', do:

make fuse flash

