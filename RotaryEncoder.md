# Introduction #

This page explains how to install the rotary encoder on the Telemetrez board.


# Details #

Your rotary encoder will have 4 pins (usually).  2 called A and B.  These connect to the rotary part.  Another connects to the switch, this is activated by pressing on the encoder.  And finally a common pin.
To connect the encoder to the Tez board, nothing but some wire is needed.  To figure out how much wire is needed, first find where you will mount the encoder.  Then measure what wire length will be comfortable, be sure not to run the wires under the gimbals, they don't fit there (I tried, that's how I know).  Now, connect the common pin to ground on the Tez board.  There is a hole marked +0V, this is a ground point.  If there is more than 1 common then connect them together.  _There might be an exception.  If you have the rotary encoder with the 4-way buttons included I think the common for the buttons needs to be separate._  Ok, next connect the switch pin to expansion hole I on the Tez.  Then A and B connect to holes G and H.  Once you get the encoder running, if it is working backwards these are the 2 connections you need to reverse.  Remount the Tez in your radio and update the software.  You will need Tez version 60 or above.  And you will need Er9x > [r781](https://code.google.com/p/9x-telemetrez/source/detail?r=781) (I will be more specific when I know, Open9x will have a version too I'm sure).