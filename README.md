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

### Build Instructions

Note I'm new to `cmake` so have really just kludged this together for the moment, however this seems to work for me:

1. Install/setup the Pico SDK and ensure the PICO environment vars are set up: `PICO_SDK_PATH`, `PICO_EXAMPLES_PATH`, `PICO_EXTRAS_PATH`, `PICO_PLAYGROUND_PATH` (I don't know if they're all needed)
2. Check out the [Melopero Perpetuo LoRa](https://github.com/melopero/Melopero_Perpetuo_Lora) project.
3. Check out this project and `cd` to the project root
4. Edit the `CMakeList.txt` to point to the correct Melopero Perpetuo LoRa `src` path!
5. `cd` to the `src` directory.
6. `mkdir build`
7. `cd build`
8. `cmake ../.. -DPICO_PLATFORM=rp2350`
9. `make`

There should now be the files `sender.uf2` and `receiver.uf2` in the `src` directory (under the `build` directory you're currently in). Then copy these onto the boards...

1. With no power (i.e. battery unplugged) plug your Melopero Perpetuo LoRa board into your computer via USB whilst holding down the "BT" button, then, for example:
2. `sudo mount /dev/sdb1 /mnt/tmp`
3. `sudo cp src/receiver.uf2 /mnt/tmp/`
4. `sync`
5. `sudo umount /mnt/tmp`

In your case you may well have a different device than `/dev/sdb1` and `/mnt/tmp` is just my choice of path, this can be whatever you want.

After this is run you should be able to attach to the USBTTY to view the serial output, in my case:

* `minicom -D /dev/ttyACM1 -b 115200`

In terms of finding the correct devices above `dmesg` is your friend.

### Disclaimer 

The code is in no way warranted to be fit for any purpose at all. In fact it is
almost certainly "buggy as hell". You have been warned.
