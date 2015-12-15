# Measuring noise with The Things UNO.

The goal of this project is to create a simple, complete and low price node that serves a real use case. The intended use case is to measure real-time noise, both accumulated noise and peak noise. 

## Instructions

### Hardware
 * Get a The Things UNO
 * Get a [Adafruit Silicon MEMS Microphone](https://www.adafruit.com/products/2716)
 * Connect Vin (MEMS mic) to 3v3 (Arduino), GND (MEMS mic) to GND (Arduino) and Vin (MEMS mic) to A0 (Arduino)
 * Connect the Things UNO with your computer

### Software
 * [Install Arduino IDE](https://www.arduino.cc/en/Main/Software) and connect your The Things UNO to your computer.
 * Ensure that in the IDE 'Tools -> Board' is set to 'Arduino Leonardo' (otherwise compilation will fail, as it will not recognize Serial1)
 * Clone this repository:

        $ git clone git@github.com:matthiaszimmermann/ttn_zh_noise.git

 * Open the Arduino IDE and then open the [noise monitor sketch](noise_monitor/noise_monitor.ino).
 * Open the `LoRa.cpp` file and assign a new device address to your device on the line `Serial1.write("mac set devaddr 5A480101\r\n");`. For TTN Zurich, please use the address space 5A4801xx (i.e. replace `xx` with an hex value, eg. `03`).
 * Compile and upload the code.

## Running the Sketch

Running the sketch should periodically output noise level information as shown below:
```
--------------------------
sum: 61 time_delta: 14118 noise_acc_norm: 4.32 min: 2 max: 3 jsonData: 'info': 'ttn-zh-noise', 'version': 'v0.1', 'acc': '4.32', 'max': '3'
--------------------------
Sending: 'info': 'ttn-zh-noise', 'version': 'v0.1', 'acc': '4.32', 'max': '3'
RN2483 status:
mac_tx_ok
ok
```

Using the [REST client](http://thethingsnetwork.org/api/v0/nodes/5A480103/), the received packages then look like this:

```
{
    "time": "2015-12-15T16:47:40.123Z",
    "data_plain": "'info': 'ttn-zh-noise', 'version': 'v0.1', 'acc': '2.04', 'max': '4'",
    "gateway_eui": "FFFEB827EBA00A83",
    "data": "J2luZm8nOiAndHRuLXpoLW5vaXNlJywgJ3ZlcnNpb24nOiAndjAuMScsICdhY2MnOiAnMi4wNCcsICdtYXgnOiAnNCc=",
    "data_raw": "QAMBSFoABQABDpkrNkdj/GzUI8glcD0xnxyNigQnQVdEgN8ZCeWk6pR76m6sU67uLHgZpoUJbpU1O19KOCIFOAF3O2m+EynMab09mDcwZKAf",
    "node_eui": "5A480103"
}
```

## Credits
Based on https://github.com/gonzalocasas/thethings-uno which in turn is based on https://github.com/thingkit-ee/things_uno
