## See for a longer description: ##
http://9xforums.com/forum/viewtopic.php?f=23&t=994

## To get the hardware go here: ##
http://www.smartieparts.com/

---

## stable version: ##
[hex file](http://9x-telemetrez.googlecode.com/svn-history/r86/trunk/src/telemetrEZ.hex) v86 You will need to right-click -> save as, or something like that<br />
**features:**
  * forwards 2-way telemetry data between Frsky and 9x
  * sends THR and AIL switch updates to 9x every 20ms
  * buffers Frsky packets while sending switch packet
  * 9x programming detection, so telemetrEZ stops sending data and doesn't interfere.
  * automatically restarts sending packets 15 seconds after programming is complete
  * Clock adjustment/synchronization from ppm signal
  * Bluetooth forwarding support for Frsky packets - Now working!!
  * Rotary encoder support, see [issue 2](http://code.google.com/p/9x-telemetrez/issues/detail?id=2&can=1) for details


---

This is the final release, these have been left for production testing.  They are easy to remove when recompiling the code though.
  * IO\_A is pulled low while Frsky packets are being sent to the 9x
  * IO\_E toggles every 5ms with the system timer
  * IO\_D toggles every time through the main loop
  * IO\_C will go high when programming is detected, it will also pulse when the internal oscillator is adjusted
  * Flashing LED on IO\_J for production test, this LED only flashes for 5 seconds.