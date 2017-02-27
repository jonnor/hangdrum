
## TODO

Post

* Automated test using host-simulator.
Take input .log file, produce a flowtrace, load and check the trace.
* Unhardcode filenames/options in simulator.
* Expose key config parameters for simulator. Commandline? Threshold,filters
* Make the trace plotting automatically update on changing values.
Listen to file change on disk?

Sanity

* Setup Travis CI, run host tests and device build there

Prototype-quality

* Move profiling code to separate sketch
* Test on the real hardware

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
Ideally with a high-speed camera, so one can.
