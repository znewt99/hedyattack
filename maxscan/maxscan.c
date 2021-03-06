/*
HedyAttack - Tools for identifying and analyzing frequency hopping spread spectrum(fhss) implementations.
Copyright (C) 2011  Cutaway, Q, and Atlas

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

Please leave comments and recommendations at http://code.google.com/p/hedyattack
 */

#define NUM_CHANNELS_53
#define INC_CH_RSSI

#include <cc1110.h>
#include "types.h"
#include "ioCCxx10_bitdef.h"
#include "setup.h"
#include "stdio.h"
#include "string.h"
#include "pm.h"
#include "cc1111emk.h"

/* globals */
xdata channel_info chan_table[NUM_CHANNELS];
#define MAX_HOLD 1

void radio_setup() {
    //Set up like SmartRF Register View

    //IOCFG2   = 0x00;
    //IOCFG1   = 0x00;
    //IOCFG0   = 0x00;

    // Preamble for Data Transfer
    //SYNC1   = 0x0B;
    //SYNC0   = 0x0B;

    //PKTLEN   = 0xFF;
    PKTCTRL1 = 0xE5;
    PKTCTRL0 = 0x04;
    //ADDR     = 0x00;
    //CHANNR   = 0x00;

    // IF of 457.031 kHz 
    //FSCTRL1 = 0x06;
    //FSCTRL0 = 0x00;
	// IF of 457.031 kHz 
	FSCTRL1 = 0x12;
	FSCTRL0 = 0x00;

    // Configure Frequency - math included
    // freq    = ??
    // hz = freq * 1000000;
    // the frequency setting is in units of 396.728515625 Hz 
    // u32 setting = (u32) (freq * .0025206154);
    // 920 = 0x26 0x55 0x55
    // 902 = 0x25 0x95 0x55
    //FREQ2 = 0x26;
    //FREQ1 = 0x55;
    //FREQ0 = 0x55;
    
    //Data Rate
    // 270.833333 kHz: 0xE5; 0xA3; 0x10; 0x23; 0x11; 
    // 499.511719 kHz: 0xEE; 0x55; 0x73; 0x23; 0x55;
    MDMCFG4 = 0x3E; // 500 kBaud Data Rate + (500 kHz channel * .8 filter space)
    //MDMCFG4 = 0xEE; 
    //MDMCFG3 = 0x55; 
    //MDMCFG2 = 0x73; 
    //MDMCFG1 = 0x23; 
    //MDMCFG0 = 0x55; 

    //DEVIATN  = 0x16;

	// no automatic frequency calibration 
	MCSM0 = 0;
    // frequency calibration 
    //MCSM2  = 0x07;
    //MCSM1  = 0x30;
    //MCSM0  = 0x18;
    //FOCCFG = 0x17;
    //BSCFG  = 0x6C;

	// disable 3 highest DVGA settings 
	AGCCTRL2 |= AGCCTRL2_MAX_DVGA_GAIN;
    //AGCCTRL2 = 0x03;
    //AGCCTRL1 = 0x40;
    //AGCCTRL0 = 0x91;

    //FREND1    = 0x56;   // Front end RX configuration.
    //FREND0    = 0x10;   // Front end RX configuration.

    // frequency synthesizer calibration 
    FSCAL3 = 0xEA;
    FSCAL2 = 0x2A;
    FSCAL1 = 0x00;
    FSCAL0 = 0x1F;

    // "various test settings" 
    TEST2 = 0x88;
    TEST1 = 0x31;
    TEST0 = 0x09;

    // PA output power setting.
    //PA_TABLE7 = 0x00;   
    //PA_TABLE6 = 0x00;   
    //PA_TABLE5 = 0x00;   
    //PA_TABLE4 = 0x00;   
    //PA_TABLE3 = 0x00;   
    //PA_TABLE2 = 0x00;   
    //PA_TABLE1 = 0x00;   
    //PA_TABLE0 = 0xC0;   
}

/* freq in Hz */
void calibrate_freq(u32 freq, u8 ch) {
        /* the frequency setting is in units of 396.728515625 Hz */
        //u32 setting = (u32) (freq * .0025206154);
        u32 setting = (u32) (freq * FREQ_MULTIPLIER);
        
        FREQ2 = (setting >> 16) & 0xff;
        FREQ1 = (setting >> 8) & 0xff;
        FREQ0 = setting & 0xff;

        RFST = RFST_SCAL;
        RFST = RFST_SRX;

        /* wait for calibration */
        sleepMillis(2);

        /* store frequency/calibration settings */
        chan_table[ch].freq2 = FREQ2;
        chan_table[ch].freq1 = FREQ1;
        chan_table[ch].freq0 = FREQ0;
        chan_table[ch].fscal3 = FSCAL3;
        chan_table[ch].fscal2 = FSCAL2;
        chan_table[ch].fscal1 = FSCAL1;

        /* get initial RSSI measurement */
        chan_table[ch].ss = (RSSI ^ 0x80);
        chan_table[ch].max = 0;

        RFST = RFST_SIDLE;
}

/* Calibrate the 900 MHz Radio */
void radio_calibration(void) {
    u32 min_hz;
    u16 chan;

    /* doing everything in Hz from here on */
    min_hz = MIN_900 * 1000000;

    for (chan = 0; chan <= NUM_CHANNELS; chan++){
        calibrate_freq((min_hz + (HZ_CHAN_SPACE * chan)),chan);
    }
    
    // Long flash to show we are running
    //led_tick(1,PAUSE);
}

/* tune the radio using stored calibration */
void tune(u8 ch) {
    FREQ2 = chan_table[ch].freq2;
    FREQ1 = chan_table[ch].freq1;
    FREQ0 = chan_table[ch].freq0;

    FSCAL3 = chan_table[ch].fscal3;
    FSCAL2 = chan_table[ch].fscal2;
    FSCAL1 = chan_table[ch].fscal1;
}

void main(void) {

    //delays
    u16 action_pause;
    u16 sh_action_pause;
    u16 config_interval;

    // channel storage
    u16 ch;
    //u8 chan_rssi;

    u16 min_chan;
    u16 max_chan;

    xtalClock();
    setIOPorts();
    radio_setup();
    radio_calibration();

    //delays
    action_pause = 50;
    sh_action_pause = 10;
    config_interval = 5;

    min_chan = 0;
    max_chan = NUM_CHANNELS; // shouldn't subtract one because we want 0 thru 52

    while (1) {

        // Quick flash to show we are running
        led_tick(1,XSH_PAUSE);

        for (ch = min_chan; ch < max_chan; ch++) {

            /* tune radio */
            tune(ch);
            //sleepMillis(config_interval);
            // Start RX
            RFST = RFST_SRX;
            sleepMillis(config_interval);

            /* read RSSI */
            chan_table[ch].ss = (RSSI ^ 0x80);
            chan_table[ch].max = MAX(chan_table[ch].ss, chan_table[ch].max);

            /* end RX */
            RFST = RFST_SIDLE;

            // Sleep necessary to allow cc1111 to sync
            sleepMillis(sh_action_pause);

        }
    }
}
