What does the TelemetrEZ do?

# Introduction #

The TelemetrEZ board simplifies the upgrade to telemetry on a 9x radio running custom firmware.  It goes between the 9x processor and the Frsky module, so it can use the serial communication lines.
First, some background.  The Frsky module communicates to the 9x through serial communication lines.  On a stock board 2 switches are hooked up where the hardware serial port is connected in the 9x processor.  It used to be if you wanted access to the serial data lines you had to remove a couple of resistors to disconnect the switches from those pins.  Then you had to reconnect those switches to unused processor pins so they could still be used.  This required soldering to some very tiny processor pins.  The wires don't get a lot of surface area to connect to, so they are prone to becoming detached.  It was also very easy to short those pins to each other or other adjoining pins.
Frsky modules communicate in packets.  The TelemetrEZ also communicates via packets.  So the TelemetrEZ buffers Frsky packets and periodically inserts it's own packets in the serial line to the 9x.