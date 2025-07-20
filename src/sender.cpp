/**
 * Melopero Perpetuo LoRa - Allotment Telemetry Sender
 *
 * This code began life as the Melopero sample code the Perpetuo LoRa board.
 * This can be found here: https://github.com/melopero/Melopero_Perpetuo_Lora
 *
 * As per their code I choose to continue the MIT licencing for this.
 *
 * This is the code for a LoRa reciever note that collects the data from
 * one or many LoRa sender nodes which transmit sensor data from various
 * points on an allotment or similar environment where you may want to
 * record sensor data. The sensor data is output via the serial console
 * so that a host computer can then do whatever is needed with it (i.e.
 * send out to IP network via MQTT, or place into a database, etc.)
 *
 * DISCLAIMER: The code is in no way warranted to be fit for any purpose at
 * all. In fact it is almost certainly "buggy as hell". You have been warned.
 *
 * Yvan Seth <allotment.sensors@seth.id.au>
 * https://yvan.seth.id.au/tag/lora.html
 */
#include <cstdio>
#include <vector>
#include <numeric>
#include <cmath>
#include <algorithm>
#include <string>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "MeloperoPerpetuo.h"

// The number of times to take an ADC reading to get an average ADC reading
static const int ADC_SAMPLE_COUNT = 16;

/**
 * Read a given ADC value, returns a voltage value.
 *
 * Actually reads ACD_SAMPLE_COUNT values and returns an average after an
 * outlier elimination filter. I don't know how necessary this is on the
 * RP2350 platform, but also it probably doesn't hurt aside from a little
 * more power usage I guess.
 */
float readADCVoltage( int adc ) { std::vector<uint16_t> values;

    // get ADC readings
    adc_select_input( adc );
    for ( int i = 0; i < ADC_SAMPLE_COUNT; ++i ) {
        uint16_t adc_val = adc_read(); 
        values.push_back( adc_val );
    }

    // calculate standard deviation
    float avg = std::accumulate( values.begin(), values.end(), (float)0.0 ) / values.size();
    float sd = 0.0;
    for ( int i = 0; i < values.size(); ++i ) {
        sd += pow( values[i] - avg, 2);
    }

    std::vector<uint16_t> filtered;
    if ( 0 == sd ) {
        filtered = values;
    } else {
        // eliminate outliers here
        std::copy_if( values.begin(), values.end(), std::back_inserter(filtered), [=](uint16_t val) {
            bool pass = abs( (float)val - avg ) < (3 * sd);
            if ( ! pass ) {
                printf( "rejected: %d\n", val );
            }
            return pass;
        });
    }

    // calculate the average of remaining values
    float adc_avg = std::accumulate( filtered.begin(), filtered.end(), 0.0 ) / filtered.size();

    // convert to pin voltage
    float adc_volts = adc_avg * (3.3f / (1 << 12));

    return adc_volts;
}

// This is our data transfer/packet struct
struct txdata {
    uint16_t options; // options as defined page 42: https://www.embit.eu/wp-content/uploads/2020/10/ebi-LoRa_rev1.0.1.pdf
    uint16_t dest; // destination id, 0xFFFF for broadcast
    uint8_t charge_state; // the charge status as supplied by Melopero library
    float mcu_temp; // the internal RPi temperature sensor value
    float vbat; // battery circuit voltage - charging voltage or battery
    float vin; // supply voltage, i.e. USB, solar, or battery
};

// Serialisation functions
uint8_t* serialise_u8(uint8_t* buf, uint8_t val) {
    buf[0] = val;
    return buf + 1;
}
uint8_t* serialise_u16(uint8_t* buf, uint16_t val) {
    buf[0] = val >> 8;
    buf[1] = val;
    return buf + 2;
}
uint8_t* serialise_u32(uint8_t* buf, uint32_t val) {
    buf[0] = (val & 0xff000000) >> 24;
    buf[1] = (val & 0x00ff0000) >> 16;
    buf[2] = (val & 0x0000ff00) >> 8;
    buf[3] = (val & 0x000000ff);
    return buf + 4;
}
size_t serialise_txdata(struct txdata* txd, uint8_t* buf) {
    uint8_t* bufptr = buf;
    bufptr = serialise_u16(bufptr, txd->options);
    bufptr = serialise_u16(bufptr, txd->dest);
    bufptr = serialise_u8(bufptr, txd->charge_state);
    float mcu_temp = txd->mcu_temp;
    bufptr = serialise_u32(bufptr, *((uint32_t*)(&mcu_temp)));
    float vbat = txd->vbat;
    bufptr = serialise_u32(bufptr, *((uint32_t*)(&vbat)));
    float vin = txd->vin;
    bufptr = serialise_u32(bufptr, *((uint32_t*)(&vin)));
    return (bufptr - buf);
}

// Main function
int main() {
    stdio_init_all();  // Initialize all standard IO

    MeloperoPerpetuo melopero;
    melopero.init();  // Initialize the board and peripherals

    melopero.led_init();
    melopero.blink_led(2, 500);

    melopero.sendCmd(0x01);
    sleep_ms(500);
    printf("response to deviceId\n");
    melopero.printResponse();
    
    // LoRaEMB operating mode configuration sequence:
    // Stop Network
    // Set Network preferences 
    // Set Output power
    // Set Operating channel
    // Set Network address
    // Set Network ID
    // Set Energy save mode (the default value is TX_ONLY, set RX_ALWAYS)
    // Start Network

    melopero.stopNetwork();
    sleep_ms(500);
    printf("response to stopNetwork\n");
    melopero.printResponse();

    melopero.setNetworkPreferences(false, false, false);
    sleep_ms(500);
    printf("response to setNetworkPreferences\n");
    melopero.printResponse();

    melopero.setOutputPower(0x0a);  // Example power level
    sleep_ms(500);
    printf("response to setOutputPower\n");
    melopero.printResponse();
    
    melopero.setOperatingChannel(1, SPREADING_FACTOR_7, BANDWIDTH_125, CODING_RATE_4_5);  // Example channel
    sleep_ms(500);
    printf("response to setOperatingChannel\n");
    melopero.printResponse();

    uint16_t network_address = 0x1234;
    melopero.setNetworkAddress(network_address);  // Example network address
    sleep_ms(500);
    printf("response to setNetworkAddress\n");
    melopero.printResponse();

    uint8_t network_id[] = {0x00, 0x01};  // Example network ID
    melopero.setNetworkId(network_id, sizeof(network_id));
    sleep_ms(500);
    printf("response to setNetworkId\n");
    melopero.printResponse();

    melopero.setEnergySaveMode(ENERGY_SAVE_MODE_TX_ONLY);
    sleep_ms(500);
    printf("response to setEnergySaveModeRxAlways\n");
    melopero.printResponse();

    //melopero.setEnergySaveModeRxAlways();
    melopero.startNetwork();
    sleep_ms(500);
    printf("response to startNetwork\n");
    melopero.printResponse();


    // and GO!
    adc_init();
    adc_gpio_init(26);
    adc_gpio_init(27);
    adc_set_temp_sensor_enabled(true);
    melopero.enablelWs2812(true);
    while (1) {

        ////////////////////////////////////////////////////////
        // do a litte dance

        // simple LED on
        gpio_put(23, 1); 

        ////////////////////////////////////////////////////////
        // read sensor values
        printf("\n============================================\n");

        // the sensor data struct
        struct txdata txd = {};
        // set header values
        txd.options = 0;
        txd.dest = 0xFFFF; // broadcast "address"

        // print out the battery charging state 
        txd.charge_state = melopero.getChargerStatus();
        printf("Battery: %d (", txd.charge_state);
        if (melopero.isCharging()) { 
            printf("charging)\n");
            // yellow
            melopero.setWs2812Color(255, 255, 0, 0.1);  
        } 
        else if (melopero.isFullyCharged()) {
            printf("charged)\n");
            // green
            melopero.setWs2812Color(0, 255, 0, 0.05); 
        } 
        else if (melopero.hasRecoverableFault()) {
            printf("fault: recoverable)\n");
            // blue
            melopero.setWs2812Color(0, 0, 255, 0.05);  
        } 
        else if (melopero.hasNonRecoverableFault()) {
            printf("fault: non-recoverable)\n");
            // red
            melopero.setWs2812Color(255, 0, 0, 0.1);  
        }
        // NOTE: there seems to be an error in the charging on these
        // boards where it never reaches full charge and then hits a 
        // timeout and enters non-recoverable error, but then this is
        // reset by external power loss (i.e. no light on solar
        // overnight) - though this doesn't help if you're using
        // solar+battery as the external power source.
        // 
        // NOTE2: if there is no battery plugged in this just flips
        // between charging and charged status.

        // flip the enable on the VSEN on/off each iteration, this is just
        // testing it can turn an LED on/off if you like, or enable/disable
        // other external hardware (thus saving power, only power on sensors
        // to read data every 5 minutes, say...)
        int vsen = gpio_get( 0 );
        if ( vsen ) {
            printf("vsen off\n");
            gpio_put( 0, 0 );
        } else {
            printf("vsen on\n");
            gpio_put( 0, 1 );
        }

        // check the temperature of the RP2350
        float voltage = readADCVoltage( 4 );
        txd.mcu_temp = 27.0 - ((voltage - 0.706) / 0.001721);
        printf( "RP2350 Temperature: %0.2f C\n", txd.mcu_temp );

        // read voltage on ADC0 (battery voltage sense)
        float adc0_v = readADCVoltage( 0 );
        txd.vbat = (adc0_v * (98600 + 149100)) / 149100; // measured values of volage divider 
        printf( "Battery Voltage: %0.2fV\n", txd.vbat );

        // read voltage on ACD1 (supply voltage sense)
        float adc1_v = readADCVoltage( 1 );
        txd.vin = (adc1_v * (98600 + 14890)) / 14890; // measured values of volage divider 
        printf( "Supply Voltage: %0.2fV\n", txd.vin );

        ////////////////////////////////////////////////////////
        // send some data
        uint8_t sendbuf[sizeof(txdata)]; // at least big enough...
        size_t data_length = serialise_txdata(&txd, sendbuf);
        printf( "Sending Data:\n  length=%d\n  data={", data_length );
        for (size_t i = 0; i < data_length; i++) {
            printf("0x%02X ", sendbuf[i]);
        }
        printf("}\n");
        melopero.transmitData(sendbuf, data_length);

        sleep_ms(500); // seems a bit kludgy?
        printf("response to transmitData\n");
        melopero.printResponse();

        // simple LED off
        gpio_put(23, 0);

        // snoozZzZzZzZzzze
        sleep_ms(5000);
    }

    return 0;
}
