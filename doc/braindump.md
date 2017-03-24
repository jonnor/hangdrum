
## TODO

Quick experimentation

* Make the trace plotting automatically update on changing values.
Listen to file change on disk?
* A file that declares current "experiments" and allows manipulating them

Production-quality

* Automated on-device test.
Take input .log, sends to device over serial.
Device produces USB MIDI, host checks messages against expectation 
* Fix timeouts when pads not connected.
Either needs to report an error, or timeout in a way that does not affect usage.
* Watchdog enabled. Triggering should show error, maybe by an artificial boot delay

Mass-production

* Host program for doing final QA.
Display test procedure to be performed (manually), record device responses, store into database 

## Testing

Possible sources of variation

* Area being hit/touched. Fingertip versus fingerbalm. Confirmed very big effect.
Values maybe around proportional with surface area.
* Material between electrode and finger. Confirmed significant effect.
Thin vinyl higher values and larger difference when pressing, compared to thicker silicone or thicker plastic.
* Air humidity. Probable significant effect.
Emulated by wetting pad with a towel. Much larger values when pressed, over 2x++. Sensor read timeouts happen very frequently.
Sensitivity may need to be adjusted to compensate.
* Finger humidity. Tested fingers wetted with water, fingers covered in chalk, normal. Indicates minor effect,
possibly lower than testing error due to surface area variation.
* Electromagnetic noise. Tested lying in pile of AC power and computer PSUs. Indicates no significant effect.
* Grounding. Known theoretically to have an influence. Tested with laptop charger connected/not-connected. Indicates no significant effect.
* Mechanical adhesion of cover tape to electrode. Not tested.
Could test very loosely badly stuck versus well/tightly stuck.
* Electrical contact of adhesive copper. During assembly, or drying out over time. Not tested.
* Variation in microcontroller inport 1/0 threshold. Not tested, not planned.

Cross-talking between sensors

* 3-finger hit. On pad. Adjacent pad(s). Opposite pad.
* 1-finger hit. "="
* Palm touching. "="

## Ideas

### CC messages

It may be interesting to send the values of the capacitive sensors of the pads as MIDI CC messages.
Either for the entire instrument, or for some pads. This would allow to modulate volume, filters
Might want to have another sensitivity though, to make more use of the distance/proximity sensing of the sensors rather than touch.

### Aftertouch

Would be a different interpretation of input values when in the `StayOn` state.
Probably makes sense to have a relatively slow filter / high smoothening on this value.
Thus it probably does not need a high sensor readout rate either.

### Velocity-sensitivity

Can one get `velocity` sensitivity with only capacitive sensors?
Traditional electronic keyboards measure velocity by measuring *how fast the key is hit*.
For instance using two microswitches.

In order to do this would need high frequent data.
At 10ms+ the transition between off and on is typically done in 1-2 samples.
Would have to record a bunch of test data of hitting at different velocities.
Using a larger area to hit with could also affect velocity. Fingertip versus underside of finger, versus 3 fingers.

### Increasing sensor readout rate

With `20` samples per sensor, and 10 sensors using a 1 megaohm resistor, sensor readout took around `10 milliseconds`.
The resulting reading is the `sum` of the samples, `minus` a periodically reset `minimum` (leastAmount).

The long time to read sensors significantly limits the frequency of input values.
There is also a delay between when the first and last sensor is read in each iteration of the mainloop.

The easy way to increase the rate would be to reduce number of samples.
Reducing from `20` to `4` should bring the time down by factor 5, to `2 ms`.
A challenge is that the value of the reading will go down by a similar factor, possibly causing worse resolution.
Perhaps one can compensate by changing the resistor value such that each charge cycle is slightly longer?

The calculation time is estimated to be `< 1 millisecond`, including several EWA filters.
How long it takes to send the MIDI messages?

How fast can we record data from inputs? Note: Should change to use absolute time instead of just the readout delay.
Should be able to stream a single sensor at 1kHz+, or sub-1ms mainloop, with 8bytes ASCII encoding.
`115200/(10*8) = 1440`.
If using a binary encoding with 1 byte per sensor, 4 bytes, 1 byte marker, should be able to stream 500Hz / 2ms mainloop;
`115200/(10*15*1) = 768`.

Some [forum post](https://forum.arduino.cc/index.php?PHPSESSID=lf8qg0kso6b4e3ivpq4brrjd15&topic=108599.30)
testing indicates the Leonardo can transfer at `40 000 bytes/second`. This is 3-4 times what is stipulated above.
[Benchmarks of Teensy chips]((https://www.pjrc.com/teensy/usb_serial.html) suggest the AVR there can do above `600 kbytes/second`!


### Parallel sensor readout 

The `CapacitiveSensor` library reads each capacitive pad serially, one after the other.
In principle, a lot of the time should be spent waiting for the pins to change state.
Since we have 10 sensors, if we can parallelize the readout, can theoretically approach a 10x speedup.

If sharing the charge/send pin and reading in parallel, need to wait for the slowest pin?
Or would have a low-valued timeout/limit?

Is this possible without causing significant crosstalk?
What is the amount of crosstalk right now, with serial readout?

Should probably limit the pins which can be read in parallel to be on the same port.
A bitmask can be used to select pins in the port should be read/not.
Can then use a couple of instances to use non-trivial readout patterns,
like reading everything except for 1 pin in middle.

### Constant-time readout

`CapacitiveSensor` takes a variable amount of time to return, depending on the capacitance of the attached pad.
Basically it counts how many clock cycles it takes until the port changes state after having been charged,
and repeats this for `N samples`.
Having jitter is not entirely ideal for discrete digital filters.
In the worst case it could give noticable non-deterministic latency for the performer. How many milliseconds would it take?

One could instead cycle pins for a fixed number of time/clockcycles, and count how many times each pin manages
to complete the charge/discharge loop, and report this number as the result.

Although what matters is the total time between readout, which is also influenced by computation and by sending messages.
Perhaps one could busyloop until a certain time has passed? Say 2 ms? Like in crypography.
Toggling a pin in the mainloop should allow using an oscilloscope to check the frequency/variation.
Can count the number of times the deadline is exceeded per some time period, and consider it an error if it exceeds a threshold.

## Wanted tooling

### Syncronized video-stream

Ability to record a syncronized video-stream together with the capacitive sensor values.
If recording is done by a single host, can do rough sync there.
Use an LED "clapper" to do higher fidelity syncronization?
Ideally with a high-speed camera, so one can really see the details of how the fingers move on the pads.

If we also have an audio stream, can we use the audible taps as guides for when we should have detections?


## References

Capacitive touch sensors

* Linear Touch slider. Using
* Rotary/radial slider.
[Youtube vid](https://www.youtube.com/watch?v=kWCsBwKYi9E). AVR software-based linear and rotary slider, shows the PCB patterns.
[Intro to capacitive sensing](http://patternagents.com/news/2013/11/20/capacitive-sensing-introduction.html), OSHW touch widgets library. [Eagle files](https://github.com/PatternAgents/PCB_Libraries/tree/master/eagle6/lbr).
* Liquid level sensing.
[Instructables: How Capacitive Liquid Level Sensors Work](http://www.instructables.com/id/Building-a-Capacitive-Liquid-Sensor/).
Using foil outside a water bottle as the electrodes. Includes some (very basic) Arduino code.
A suggestion in a Youtube video comment was to have a separate capacitance sensor at the bottom of tank, and use this as a reference
for comparing the height value to. This may be able to compensate a bit for changes in temperature, water composition etc.
* [Soundplane 8x8](http://madronalabs.com/DIY). Multitouch pad.
Uses 8 rows x 8 columns on top of eachother, being capacitively coupled.
Audio-rate frequencies injected into each of the rows, and then read out from the columns.
* DisneyResearch: [Touche](https://www.youtube.com/watch?v=E4tYpXVTjxA),
gesture-sensing based on swept-frequency capacitive reading.
Video shows a number of applications of objects and even the body as a sensor unit.
* DisneyResearch: [EM-Sense](https://www.youtube.com/watch?v=fpKDNle6ia4),
object identification based on their capacitive profiles, read by a smartwatch.
* FreeScale: [AN3863 Designing Touch Sensing Electrodes](http://cache.freescale.com/files/sensors/doc/app_note/AN3863.pdf).
Gives many recommendations on how to shape and lay out touch-sensing electrodes on flexible or regular PCBs.
* NXP: [Proximity Capacitive Sensor Technology for Touch Sensing Applications](http://cache.freescale.com/files/sensors/doc/white_paper/PROXIMITYWP.pdf). Shows using multiple electrodes, where one electrode is charged, and the finger creates a capacitive path to another electrode connected to ground. Brief mention of 8-pad rotary endocder, liquid sensing
* Rotary capacitive encoder / capacitive resolver.
[Open Source Ecology](http://opensourceecology.org/wiki/Rotary_encoder), design for a 48 step, 100x iterpolating resoler, 0.075 degrees resolution.
Includes some Arduino code, and a SPICE electronics model.

## Blogging notes

Around 'understanding embedded systems'

### Can we support more advanced features?

On/off triggering can make for a usable & fun musical intrument, but the expressiveness is somewhat limited.

Existing MIDI drums and keyboards often send a velocity for notes triggered. This is a value representing how how hard a pad or key is being hit.
This is primarily mapped to the volume of the note, but often other qualities of the sound changes a bit too.
For instance, a trumpet (or an emulation thereof) played hard sounds distorted and has more overtones compared to when played softly.

Old-school keyboards would use two microswitches, and use the time between them to determine velocity.


Theoretically we can track how quickly a capacitive sensor pad is being hit.
But can we sample the sensor often enough, with a low enough noise level to have the required data?
And can we do the required filtering in real-time on the processor?

Do we have the required input data?
Only 1-3 values from not touching to touching. Not really enough.

### End-product QA

The tooling built could be reused in manufacturing QA for testing each completed unit.
A testing procedure could specify to tap each of the sensor pads, and the system recording and verifying that the input signals are within expected ranges and that.

### More features

Some keyboards support Aftertouch, which allows to modulate a value after triggering a note, usually by pressing the keys down harder.
This can be mapped to any parameter of the synthesizer, like changing pitch, amplitude or modulating a filter or low-frequency oscillator.
When playing a violin sound it could for instance be mapped to vibrato.
https://www.youtube.com/watch?v=AsFQBm5X9mw

### Prevalence

98% of microprocessor chips go into embedded devices. 1% to personal computers
http://www.icinsights.com/news/bulletins/Microcontroller-Unit-Shipments-Surge-But-Falling-Prices-Sap-Sales-Growth/

### "debugging"

I don't like this term much.
Suggests that written code code mostly works, except for some minor bugs.
But often the design is flawed. Based on assumptions or misunderstandings.
Sometimes it can be obvious (when using system) that it doesnt work.
But it can also be something which only occurs occationally or rarely.
Sometimes people make workaround so you never hear about it.
Learning (about the problem, about the solution).
Increasing understanding

### Tractability of embedded systems

Dedicated/fixed functionality is usually the savior,
that makes problem tractable with all the other constraints.
Complexity is growing however. "smart devices", "internet of things"

### More on cap sensors

Capacitive sensors are nice because they are very simple in principle, and yet can be adapted to many different cases.
The easiest usage is for a on/off switch, but they can also be used easily as a distance/proximity sensor.
More advanced input devices include linear sliders and rotary/radial sliders (like on the iPod).
And of course the touchscreen of your mobile phone or tablet.
On the bleeding edge is using capacitive sensing to make everyday objects become gesture sensors, and for physical object detection.

The CapacitiveSensor library does a couple of things to help with this.
A number of samples are read and integrated by summing them.
It keeps track of the minimum capacitance seen, and subtracts this.

The capacitor stores electrical energy. By charging it, and measuring how long it takes to discharge we can.

There are also some challenges. First, unless your microcontroller has dedicated capacitive support the reading the sensor values must be done using some pretty low-level bitbanging.
The sensor data is likely to be quite noisy, possibly from cross-talk from other sensors, induced electromagnetic interference.
There will be variation in the readings depending of temperature and humidity.
Values will be different if your device is on battery power or connected earth-ground through a powersupply connected to mains power.

The contact sensor can be anything which is conductive, and in any shape.
It can be covered, also by non-conductive materials.
No moving parts.
And it can be
Use copper foil tape to integrate it into the body of your device.
Or trace it directly into the PCB of your electronics.
Or use the PCB as the casing! Linky

It also requires very little external hardware or special features from the microcontroller.
Two resistors. Reading can be done in software.

### Lack of online information

One challenge compared to many other aspects of software programming is that there is way less (good)
information available online. Embedded software development, compared to say web development,
happens in a more traditional, inside-a-company, university-educated-people.
A lot of "tribal" knowledge doesn't make it out there. 
Changing with Arduino, Raspberry Pi, physical computing becoming part of education, AdaFruit. Hacker and makerspaces.

