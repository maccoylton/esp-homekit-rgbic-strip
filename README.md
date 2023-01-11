![GitHub All Releases](https://img.shields.io/github/downloads/maccoylton/esp-homekit-rgbic-strip/total) 
![GitHub Releases](https://img.shields.io/github/downloads/maccoylton/esp-homekit-rgbic-strip/latest/total)
# esp-homekit-rgbic-strip
ESP homekit firmare for RGBIC strips


This firmware provides homekit support for all type of individually addressable lead strips, incluing WS2812, WS2812B, WS2813 & WS2815 that use an IC to make each pixel individually adddressable. It also provides over the air updates. For more detaisl on how to instlal the formware please look at the Wiki. 

Standard RGB and on off controlls are provided and availabel in al HOmekit apps. TO use the modes, you need to use an APP lile Eve, whihc siupport custo (non Apple standard) characteristics. 


# Effects

The effect are selected by number as follows:- 

 0.   Static - No blinking. Just plain old static light.
 1.   Blink - Normal blinking. 50% on/off time.
 2.   Breath - Does the "standby-breathing" of well known i-Devices. Fixed Speed.
 3.   Color Wipe - Lights all LEDs after each other up. Then turns them in that order off. Repeat.
 4.   Color Wipe Inverse - Same as Color Wipe, except swaps on/off colors.
 5.   Color Wipe Reverse - Lights all LEDs after each other up. Then turns them in reverse order off. Repeat.
 6.   Color Wipe Reverse Inverse - Same as Color Wipe Reverse, except swaps on/off colors.
 7.   Color Wipe Random - Turns all LEDs after each other to a random color. Then starts over with another color.
 8.   Random Color - Lights all LEDs in one random color up. Then switches them to the next random color.
 9.   Single Dynamic - Lights every LED in a random color. Changes one random LED after the other to another random color.
 10.  Multi Dynamic - Lights every LED in a random color. Changes all LED at the same time to new random colors.
 11.  Rainbow - Cycles all LEDs at once through a rainbow.
 12.  Rainbow Cycle - Cycles a rainbow over the entire string of LEDs.
 13.  Scan - Runs a single pixel back and forth.
 14.  Dual Scan - Runs two pixel back and forth in opposite directions.
 15.  Fade - Fades the LEDs on and (almost) off again.
 16.  Theater Chase - Theatre-style crawling lights. Inspired by the Adafruit examples.
 17.  Theater Chase Rainbow - Theatre-style crawling lights with rainbow effect. Inspired by the Adafruit examples.
 18.  Running Lights - Running lights effect with smooth sine transition.
 19.  Twinkle - Blink several LEDs on, reset, repeat.
 20.  Twinkle Random - Blink several LEDs in random colors on, reset, repeat.
 21.  Twinkle Fade - Blink several LEDs on, fading out.
 22.  Twinkle Fade Random - Blink several LEDs in random colors on, fading out.
 23.  Sparkle - Blinks one LED at a time.
 24.  Flash Sparkle - Lights all LEDs in the selected color. Flashes single white pixels randomly.
 25.  Hyper Sparkle - Like flash sparkle. With more flash.
 26.  Strobe - Classic Strobe effect.
 27.  Strobe Rainbow - Classic Strobe effect. Cycling through the rainbow.
 28.  Multi Strobe - Strobe effect with different strobe count and pause, controlled by speed setting.
 29.  Blink Rainbow - Classic Blink effect. Cycling through the rainbow.
 30.  Chase White - Color running on white.
 31.  Chase Color - White running on color.
 32.  Chase Random - White running followed by random color.
 33.  Chase Rainbow - White running on rainbow.
 34.  Chase Flash - White flashes running on color.
 35.  Chase Flash Random - White flashes running, followed by random color.
 36.  Chase Rainbow White - Rainbow running on white.
 37.  Chase Blackout - Black running on color.
 38.  Chase Blackout Rainbow - Black running on rainbow.
 39.  Color Sweep Random - Random color introduced alternating from start and end of strip.
 40.  Running Color - Alternating color/white pixels running.
 41.  Running Red Blue - Alternating red/blue pixels running.
 42.  Running Random - Random colored pixels running.
 43.  Larson Scanner - K.I.T.T.
 44.  Comet - Firing comets from one end.
 45.  Fireworks - Firework sparks.
 46.  Fireworks Random - Random colored firework sparks.
 47.  Merry Christmas - Alternating green/red pixels running.
 48.  Fire Flicker - Fire flickering effect. Like in harsh wind.
 49.  Fire Flicker (soft) - Fire flickering effect. Runs slower/softer.
 50.  Fire Flicker (intense) - Fire flickering effect. More range of color.
 51.  Circus Combustus - Alternating white/red/black pixels running.
 52.  Halloween - Alternating orange/purple pixels running.
 53.  Bicolor Chase - Two LEDs running on a background color (set three colors).
 54.  Tricolor Chase - Alternating three color pixels running (set three colors).
 55.  ICU - Two eyes looking around
