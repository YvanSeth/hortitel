# hortitel
## Horti(culture)Tel(emetry)
### Allotment Telemetry Project

**Project Goal:** Gather various sensor data from our allotment so we can observe present data and compare to past measurements.

This is implemented using the Melopero Perpetuo LoRa boards as documented in their project here: [Melopero Perpetuo LoRa](https://github.com/melopero/Melopero_Perpetuo_Lora)

The code is derived from their initial examples.

At present this reads some data from the 'sender' board and sends it via the
LoRaEMB protocol to the 'receiver board. The data sent is the Pico's internal
temperature sensor as well as voltage readings from the battery circuit and the
board input voltage (i.e. USB or solar). Note that the voltage measurement uses
an external voltage divider circuit, the resistances of this are essentially
documented in the code.

The data is sent and received by serialisation and deserialisation of some
structs.  It could well be neater to use a text format for, but I'm a bit
old-school like this.

This project is very far from being a working thing... but I'm getting it
uploaded as perhaps the more involved examples of sending and receiving data
will be useful to someone.

Eventually I plan to build this out by adding more sensors, a proper enclosure,
and an external solar/battery power-source.


### Disclaimer 

The code is in no way warranted to be fit for any purpose at all. In fact it is
almost certainly "buggy as hell". You have been warned.
