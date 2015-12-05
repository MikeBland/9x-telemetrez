This page explains the programming detection feature of the firmware

# 9x programming detection #

The programming detection feature works by monitoring the ppm line for high to low transitions.  If a transition hasn't occurred in a certain amount of time (ppm signal isn't being sent) then all communications from the TelemetrEZ to the 9x are stopped.  The Tx pin is put into a high-impedance mode so it doesn't interfere with the programming process.  The TelemetrEZ will automatically start communicating again 15 seconds after the PPM signal returns.  Also if the 9x is in simulator mode (no PPM is generated to the module), then packets will still be forwarded to the 9x.  This happens because the TelemetrEZ detects the PPM line being held low by the 9x.