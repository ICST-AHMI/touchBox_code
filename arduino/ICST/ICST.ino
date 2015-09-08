//High performance analog data acquisition on the Arduino Duemilanove, with uniform 
//sampling rate.

//    Copyright (C) 2011 Marco Civolani
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
//    Contacts:
//    Marco Civolani  - email: marcocivo@gmail.com
//    Stefano Papetti - email: stefano.papetti@zhdk.ch
//

//DESCRIPTION:
// Notice: this code was developed on the Arduino Duemilanove, but possibly applies to 
// other versions as well.
//
// Since the Arduino API (as of May 2011) doesn't enable precise control over the ADC 
// sampling rate, this code bypasses the API and instead uses hardware interrupts. 
// By acting on some hardware registers (as done at the beginning of the 
// code) the ADC can be configured to work in "free running mode".
// In this way, the ADC is driven by a hardware timer (i.e. very precise) internal to the 
// Atmel chip, which at regular intervals asks to read the analog input(s).
// The timer frequency is selectable as a sub-multiple of the CPU frequency, by means of a
// multiplying factor set by the prescaler: i.e. everything is in sync with the 16 MHz
// quartz that provides the clock signal to the Atmel chip.
// When the ADC has the acquired value in its registers, it also raises an interrupt which
// is then intercepted by the function ISR(ADC_vect). In more detail the function does 
// what follows:
// - Puts the value in ADCValue;
// - Sends ADCValue via serial (together with the channel number, forming a 2 byte 
// packet);
// - Sets the value of the new channel from which the ADC will acquire data at the next
// round (i.e. polling on the first 4 channels).
//
// Please note that this firmware version was used for testing if the sampling rate was 
// actually stable.
// This was done by probing pin 12, so there's an extra line of code used for that:
// PORTB ^= 1 << PORTB4;
// which inverts the state of pin 12 at each execution of ISR(ADC_vect).
//


#include <avr/io.h>
#include <avr/power.h>
#include <avr/interrupt.h>

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif 

int ADCValue;

void setup();
void loop();

void setup()
{ 
  Serial.begin(115200);
  
  DDRB |= 1 << PORTB4;  // the same as pinMode(12, OUTPUT);
  
  cli(); // disable interrupts while messing with their settings
  
  /********** ADC setup *********************************************************/
  
    /* internal pull-ups interfere with the ADC. disable the
    * pull-up on the pin if it's being used for ADC. either
    * writing 0 to the port register or setting it to output
    * should be enough to disable pull-ups. */
    PORTC = 0x00;
    DDRC = 0x00;

    ADCSRA = 0;
    ADCSRB = 0; // Free-running mode
    
    PRR &= ~bit(PRADC);      // Disable ADC power reduction

    ADMUX = 0<<ADLAR; // right-aligned
    ADMUX |= 1<<REFS0 | 0<<REFS1; // internal Vcc reference
    
    // All'inizio, seleziono il canale 0.
    ADMUX |= 0<<MUX3 | 0<<MUX2 | 0<<MUX1 | 0<<MUX0;
     
    // Equivalenze tra valori del prescaler e Fs (tempo di conversione 
    // a regime: 13.5 cicli di CPU).
    // Fs = 16000kHz / 13.5 / Ps
    //
    // Prescaler | ADPS2 | ADPS1 | ADPS0 |  Fs [kHz]
    // ------------------------------------------------
    //    2      |   0   |   0   |   0   |  592.59
    //    4      |   0   |   1   |   0   |  296.30
    //    8      |   0   |   1   |   1   |  148.15
    //    16     |   1   |   0   |   0   |  74.07
    //    32     |   1   |   0   |   1   |  37.04
    //    64     |   1   |   1   |   0   |  18.52
    //    128    |   1   |   1   |   1   |  9.26
    //
    // -------------------------------------------------------------
    // | ATTENZIONE: la Fs per canale è data dalla Fs calcolata    |
    // | con la tabella di cui sopra DIVISO il numero dei canali!! |
    // -------------------------------------------------------------
    //
    // La tabella sopra riportata indica i valori di Fs INTERNI
    // all'Arduino: dato che la trasmissione seriale necessita di
    // tempo di CPU, per avere il valore REALE della Fs bisogna
    // considerare l'onda quadra con duty cycle al 50% generata
    // al pin 12 tramite il presente firmware. Ovviamente la durata
    // del periodo di tale onda quadra, dipende anche dal valore del
    // prescaler. Dalle misure effettuate con l'oscilloscopio, si
    // evincono i seguenti valori.
    //
    // ___________________
    // | Prescaler = 128 |
    // |_________________|
    //
    // TX baudrate [baud] |  Ts (per 4) [ms]  |  Fs (per canale) [Hz] 
    // ---------------------------------------------------------------
    //       9600         |    2.1            |          119
    //       19200        |    1.04           |          240
    //       38400        |    0.52           |          480
    //       57600        |    0.36           |          694
    //       115200       |    0.17           |          1470
    //
    // NOTA: Per ricevere i dati durante le misure effettuate con
    // l'oscilloscopio è stata sfruttata la patch di Pd in grado
    // di scrivere i dati su file (writeToFile.pd).
    
    ADCSRA |= 1<<ADPS2;
    ADCSRA |= 1<<ADPS1;
    ADCSRA |= 1<<ADPS0;
    
    ADCSRA |= 1<<ADATE;      // Autotrigger enabled
    ADCSRA |= 1<<ADIF;       // ADC interrupt flag is cleared
    ADCSRA |= 1<<ADIE;       // ADC interrupt capability is activated

    // Free running mode
    ADCSRB |= 0<<ADTS2;
    ADCSRB |= 0<<ADTS1;
    ADCSRB |= 0<<ADTS0;
     
    ADCSRA |= 1<<ADEN;       // ADC enabled

  /******************************************************************************/
  
  sei();                     // turn interrupts back on

  ADCSRA |= 1<<ADSC;       // Conversion started  
}

void TX(int value, byte ch)
{
  byte LSB, MSB;
  int tmpVal;
  
  LSB = 0;
  MSB = B10000000 | (ch << 3);
    
  tmpVal = value & 896;  // 1110000000
  tmpVal = tmpVal >> 7; // / 128;
  MSB = MSB | tmpVal;
    
  tmpVal = value & B01111111;
  LSB = tmpVal;

  Serial.write(MSB);
  Serial.write(LSB);
}

void loop()
{
  //Serial.println("nextframe");
}

ISR(ADC_vect)
{
  cli();
  
    ADCValue = ADCL;
    ADCValue += ADCH << 8;
        
    //TX(ADCValue, ADMUX & B00001111);       // Questa sarebbe l'istruzione corretta...

    TX(ADCValue, ((ADMUX & B00001111)-1)%3); // Pezza per recuperare il valore corretto
                                             // del canale: colpa di Arduino che spedisce
                                             // il valore sul canale sbagliato
    
    // Seleziono nuovo canale di acquisizione.
    // La prossima conversione avverrà su tale canale.
    if((ADMUX & B00001111) == B00000000)
    {
      cbi(ADCSRA,ADSC);
      cbi(ADMUX,MUX3);
      cbi(ADMUX,MUX2);
      cbi(ADMUX,MUX1);
      sbi(ADMUX,MUX0);
      sbi(ADCSRA,ADSC);
    }
    
    else if((ADMUX & B00001111) == B00000001)
    {
      cbi(ADCSRA,ADSC);
      cbi(ADMUX,MUX3);
      cbi(ADMUX,MUX2);
      sbi(ADMUX,MUX1);
      cbi(ADMUX,MUX0);
      sbi(ADCSRA,ADSC);
    }
    
    else if((ADMUX & B00001111) == B00000010)
    {
      cbi(ADCSRA,ADSC);
      cbi(ADMUX,MUX3);
      cbi(ADMUX,MUX2);
      sbi(ADMUX,MUX1);
      sbi(ADMUX,MUX0);
      sbi(ADCSRA,ADSC);
    }
    
    else if((ADMUX & B00001111) == B00000011)
    {
      cbi(ADCSRA,ADSC);
      cbi(ADMUX,MUX3);
      cbi(ADMUX,MUX2);
      cbi(ADMUX,MUX1);
      cbi(ADMUX,MUX0);
      sbi(ADCSRA,ADSC);
    }
    
    // DEBUG: clock dell'ADC in uscita su pin 12: alza il pin 12 se basso,
    // altrimenti lo abbassa.
    PORTB ^= 1 << PORTB4;  // the same as digitalWrite(12, HIGH if LOW or LOW if HIGH)
    
  sei();
}

