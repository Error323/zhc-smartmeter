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
  char result;
  
  if (i < 7)
    result = ms.Match("%d+%.%d+");
  else
    result = ms.Match("000[0-2]");

  if (!result)
  {
    DEBUG("Parse error %d for %s", result, obis);
    return;
  }
  
  pos += ms.MatchStart;
  String s = telegram.substring(pos, pos + ms.MatchLength);
  DEBUG("%d = %s", i, s.c_str());
  switch (i)
  {
    case 0: msg.total_kwh_used_high = s.toFloat(); break;
    case 1: msg.total_kwh_used_low = s.toFloat(); break;
    case 2: msg.total_kwh_returned_high = s.toFloat(); break;
    case 3: msg.total_kwh_returned_low = s.toFloat(); break;
    case 4: msg.total_gas_used = s.toFloat(); break;
    case 5: msg.current_used_kwh = s.toFloat(); break;
    case 6: msg.current_returned_kwh = s.toFloat(); break;
    case 7: msg.current_kwh_tariff = s.toInt(); break;
    default: return;
  }
}


void setup()
{
  INIT_DEBUG();
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
  p1.begin(9600);
  DEBUG("Initialized");
}


void loop()
{
  uint32_t now = timer_tick_count;
  static uint8_t obis_index = 0;
  static bool telegram_ready = false;
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
  if (obis_index >= 8)
  {
    DEBUG("Sending telegram");
    tinymac_send(0, tinymacType_RawData, (const char*)&msg, sizeof(msg), 0, NULL);
    obis_index = 0;
    telegram_ready = false;
  }

  // if we are done receiving the message we parse one obis per loop cycle to
  // make sure we stay within 250 ms.
  if (telegram_ready)
  {
    parse_obis(obis_index);
    obis_index++;
  }

  // Capture serial data on the p1 port. We expect a telegram every 10 seconds
  while (p1.available())
  {
    c = (char)p1.read();
    c &= 127;
    switch (c)
    {
      case '/': 
        telegram = c;
        break;
      case '!': 
        telegram += c;
        telegram_ready = true;
        DEBUG("length = %d", telegram.length());
        break;
      default: 
        telegram += c; 
        break;
    }
  }
}
