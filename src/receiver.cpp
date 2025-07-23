/**
 * Melopero Perpetou LoRa - Allotment Telemetry Receiver
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
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "MeloperoPerpetuo.h"

static const int SAMPLES = 16;

float readADCVoltage( int adc ) {
    std::vector<uint16_t> values;

    // get ADC readings
    adc_select_input( adc );
    for ( int i = 0; i < SAMPLES; ++i ) {
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

// this is the structure of the received data
// TODO: we should support the core real data being a variable-length array
//   perhaps like: null-terminated-label, int-type-code, value-length-dep-on-type
//   or just do what normal folks do these days and make it a text format
struct rxdata {
    // the "header"
    // see LoRaEMB on page 42: https://www.embit.eu/wp-content/uploads/2020/10/ebi-LoRa_rev1.0.1.pdf
    uint16_t length; 
    uint16_t options;
    uint8_t wtf; // possibly an extra byte in the received data here? what is it? padding?
    int16_t rssi;
    uint16_t src;
    uint16_t dst;

    // our data starts here
    uint8_t charge_state; // the charge status as supplied by Melopero library - actual struct +3 bytes padding/alignment
    float mcu_temp; // the internal RPi temperature sensor value
    float vbat; // battery circuit voltage - charging voltage or battery
    float vin; // supply voltage, i.e. USB, solar, or battery

    // 1 byte checksum, simply the low byte of the sum of the previous bytes
    // not documented in the above doc for some reason, I suspect it's not our problem to validate it
    uint8_t checksum; 
};

// FIXME: error detection - format, length, etc - this code is **UNSAFE**
uint8_t * deserialise_i16(uint8_t * buf, int16_t * val) {
    uint16_t uval = 0;
    uval |= ((uint16_t)buf[0]) << 8;
    uval |= (uint16_t)buf[1];
    *val = (int16_t)uval;
    return buf + 2;
}
uint8_t * deserialise_u16(uint8_t * buf, uint16_t * val) {
    *val = 0; // just to be sure
    *val |= ((uint16_t)buf[0]) << 8;
    *val |= (uint16_t)buf[1];
    return buf + 2;
}
uint8_t * deserialise_u8(uint8_t * buf, uint8_t * val) {
    *val = (uint8_t)buf[0];
    return buf + 1;
}
// not cross-plaform, but my application doesn't need to be
uint8_t * deserialise_float(uint8_t * buf, float * val) {
    *val = 0; // just to be sure
    *((uint32_t*)val) |= ((uint32_t)buf[0]) << 24;
    *((uint32_t*)val) |= ((uint32_t)buf[1]) << 16;
    *((uint32_t*)val) |= ((uint32_t)buf[2]) << 8;
    *((uint32_t*)val) |= (uint32_t)buf[3];
    return buf + 4;
}
void deseralise_rxdata(uint8_t * buf, struct rxdata *rxd) {
    buf = deserialise_u16(buf, &(rxd->length));
    buf = deserialise_u16(buf, &(rxd->options));
    buf = deserialise_u8(buf, &(rxd->wtf));
    buf = deserialise_i16(buf, &(rxd->rssi));
    buf = deserialise_u16(buf, &(rxd->src));
    buf = deserialise_u16(buf, &(rxd->dst));
    buf = deserialise_u8(buf, &(rxd->charge_state));
    buf = deserialise_float(buf, &(rxd->mcu_temp));
    buf = deserialise_float(buf, &(rxd->vbat));
    buf = deserialise_float(buf, &(rxd->vin));
    buf = deserialise_u8(buf, &(rxd->checksum));
}

// Main function
int main() {
    stdio_init_all();  // Initialize all standard IO

    MeloperoPerpetuo melopero;

    melopero.init();  // Initialize the board and peripherals

    melopero.led_init();
    melopero.blink_led(3, 250);

    melopero.sendCmd(0x01);
    sleep_ms(500);
    printf("response to deviceId\n");
    melopero.printResponse();
    
    // LoRaEMB operating mode configuration
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
    
    melopero.setNetworkAddress(0x1235);  // Example network address
    sleep_ms(500);
    printf("response to setNetworkAddress\n");
    melopero.printResponse();

    uint8_t network_id[] = {0x00, 0x01};  // Example network ID
    melopero.setNetworkId(network_id, sizeof(network_id));
    sleep_ms(500);
    printf("response to setNetworkId\n");
    melopero.printResponse();

    melopero.setEnergySaveMode(ENERGY_SAVE_MODE_ALWAYS_ON);
    sleep_ms(500);
    printf("response to setEnergySaveModeRxAlways\n");
    melopero.printResponse();

    melopero.startNetwork();
    sleep_ms(500);
    printf("response to startNetwork\n");
    melopero.printResponse();

    adc_init();
    adc_set_temp_sensor_enabled(true);
    melopero.enablelWs2812(true);
    while (1) {
       
        // simple LED on whilst loop body is executing
        gpio_put(23, 1);

        printf("\n============================================\n");
        printf( "MCU Board State:\n" );

        ///////////////////////////////////////////////////////////////////////
        // print out the battery charging state, also set LED colour code
        printf("  Battery: %d (", melopero.getChargerStatus());
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

        // Note: If there is no battery plugged in this just flips between
        // charging and charged status. If there it a battery but no input
        // power then it seems to always report 'fully charged' so I think for
        // full power state awareness you also need to be able to check supply
        // voltage and ideally also battery/charge voltage.  I have experienced
        // some slight oddness from the charging circuit where it sometimes
        // never fully charges, hits a timeout, and reports
        // non-recoverable-error... this status is reset by flipping the
        // external power off-and-on-again.


        ///////////////////////////////////////////////////////////////////////
        // check the temperature of the RP2350
        float voltage = readADCVoltage( 4 );
        float temp = 27.0 - ((voltage - 0.706) / 0.001721); // values from RP2350 documentation
        printf( "  RP2350 Temperature: %0.2f C\n", temp );


        ///////////////////////////////////////////////////////////////////////
        // read received LoRa data, and if available print receive data
        uint8_t rxbuff[sizeof(struct rxdata)]; // at least big enough for our *expected* data
        size_t rxbuff_ptr = 0;
        bool verbose = false; 
        if (melopero.checkRxFifo(500)) { // if there is data received...
            // TODO: this really needs some sort of validation
            do {
                if (verbose) printf("rx: ");
                for (size_t i = 0; i < melopero.responseLen; i++) {
                    if (rxbuff_ptr < sizeof(rxbuff)) {
                        rxbuff[rxbuff_ptr] = melopero.response[i];
                        if (verbose) printf("0x%02X ", rxbuff[rxbuff_ptr]);
                    } else {
                        // more data than buffer holds received, we just ignore it
                        if (verbose) {
                            printf("(0x%02X) ", melopero.response[i]);
                        } else {
                            break;
                        }
                    } 
                    rxbuff_ptr++;
                }
                if (verbose) printf("\n");
            } while (melopero.checkRxFifo(500)); // Keep checking the FIFO for new data
                                             
            // NOTE: the rxbuff_ptr could be < or > actual buffer length
            printf( "Received Data Info:\n  length=%d (buflen=%d)\n  data={", rxbuff_ptr, sizeof(rxbuff) );
            uint32_t checksum = 0;
            size_t safelen = (rxbuff_ptr < sizeof(rxbuff)) ? rxbuff_ptr : sizeof(rxbuff);
            for (size_t i = 0; i < safelen ; i++) {
                if ( i < (safelen - 1) ) checksum += rxbuff[i];
                printf("0x%02X ", rxbuff[i]);
            }
            printf("}\n");
            checksum &= 0x000000ff;
            printf( "  checksum=%02X\n", checksum );

            struct rxdata rxd = {};
            deseralise_rxdata( rxbuff, &rxd );

            printf( "Deserialised Packet Info:\n" );
            printf( "  Data Length: %u bytes\n", rxd.length );
            printf( "  Options: 0x%08X (bitfield)\n", rxd.options );
            printf( "  WTF: 0x%02X (undocumented field?)\n", rxd.wtf );
            printf( "  Signal Strength: %d dBm\n", rxd.rssi );
            printf( "  Source Addr: 0x%04X\n", rxd.src );
            printf( "  Dest Addr: 0x%04X (0xFFFF is broadcast)\n", rxd.dst );
            printf( "  Payload: \n" );
            printf( "    RP2350 Temperature: %0.2f C\n", rxd.mcu_temp );
            printf( "    Charge State: %u\n", rxd.charge_state );
            printf( "    Battery Voltage: %0.2fV\n", rxd.vbat );
            printf( "    Supply Voltage: %0.2fV\n", rxd.vin );
            printf( "  Checksum: %02X\n", rxd.checksum );

        } else {
            printf("nothing in the rx fifo\n");
        }

        // simple LED off
        gpio_put(23, 0);

        // check every second for recieved data
        sleep_ms(1000);
    }

    // technically this is unreachable?
    melopero.enablelWs2812(false);
    return 0;
}
