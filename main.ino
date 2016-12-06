#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <avr/wdt.h>
#include <util/delay.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#include <MsTimer2.h>
#include <SoftwareSerial.h>
#include <Regexp.h>

#include "obis.h"
#include "board.h"
#include "phy.h"
#include "tinymac.h"

SoftwareSerial p1(7, 6, true); // RX, TX, we are only using RX
static volatile uint32_t timer_tick_count = 0;
uint32_t next_mac_tick = 0;
String telegram;
char obis[16];
telegram_t msg;
MatchState ms;

void tick()
{
	timer_tick_count++;
}

void parse_obis(int i)
{
  strcpy_P(obis, (char*)pgm_read_word(&obis_table[i]));
  int pos = telegram.indexOf(obis) + strlen(obis);
  ms.Target(&telegram[pos]);
  char result = ms.Match("%d+%.%d+|000[0-2]");

  if (!result)
    return;
  
  switch (i)
  {
    case 0: msg.gas_used_total = telegram.substring(ms.MatchStart, ms.MatchLength).toFloat(); break;
    case 1: msg.kw_used_current = telegram.substring(ms.MatchStart, ms.MatchLength).toFloat(); break;
    case 2: msg.kw_returned_current = telegram.substring(ms.MatchStart, ms.MatchLength).toFloat(); break;
    case 3: msg.kw_tariff_current = telegram.substring(ms.MatchStart, ms.MatchLength).toInt(); break;
    default: return;
  }
}

void setup()
{
  p1.begin(9600);
  telegram.reserve(MAX_TELEGRAM_SIZE);
	tinymac_params_t params;
  params.uuid = 0x8000736d65746572ull;
  params.coordinator = FALSE;
  params.flags = TINYMAC_ATTACH_FLAGS_SLEEPY | 4; // heartbeat every 16 seconds
	next_mac_tick = 0;

	MCUSR = 0;
	wdt_disable();

	EIMSK = 0;
	EICRA = 0;

	PORTB = PORTB_VAL;
	PORTC = PORTC_VAL;
	PORTD = PORTD_VAL;
	DDRB = DDRB_VAL;
	DDRC = DDRC_VAL;
	DDRD = DDRD_VAL;

  MsTimer2::set(250, tick);
  MsTimer2::start();
  sei();

	TRX_ON();

	phy_init();
	tinymac_init(&params);
}

void loop()
{
  uint32_t now = timer_tick_count;
  static uint8_t i = 0;
  static uint8_t j = 0;
  static bool done = false;
  char c;

  // Tinyhan tick must be called every 250 ms
  if ((int32_t)(now - next_mac_tick) >= 0) 
  {
    next_mac_tick = now + 1;
    tinymac_tick_handler(NULL);
  }

  // Call the tinyhan phy event handler whenever possible
  phy_event_handler();

  // if we are done parsing all the values from the telegram we can send the
  // data
  if (j >= 4)
  {
    tinymac_send(0, tinymacType_RawData, (const char*)&msg, sizeof(msg), 0, NULL);
    j = 0;
    done = false;
  }

  // if we are done receiving the message we parse one obis per loop cycle to
  // make sure we stay within 250 ms.
  if (done)
  {
    parse_obis(j);
    j++;
  }

  // Capture serial data on the p1 port. We expect a telegram every 10 seconds
  while (p1.available())
  {
    c = (char)p1.read();
    c &= 127;

    switch (c)
    {
      case '/': 
        i = 0; 
        break;
      case '!': 
        done = true;
        break;
      default: 
        telegram[i++] = c; 
        break;
    }

    i %= MAX_TELEGRAM_SIZE;
  }
}
