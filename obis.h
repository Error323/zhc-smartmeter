
#include "tinyhan_platform.h"
#include "common.h"

// max size of a telegram coming out of the P1 port
#define MAX_TELEGRAM_SIZE 512

// P1 Addresses (OBIS references) according to the dutch standard table
// definition
const char TABLE TOTAL_GAS_USED[]      = "0-1:24.2.0";
const char TABLE CURRENT_USED_KW[]     = "1-0:1.7.0";
const char TABLE CURRENT_RETURNED_KW[] = "1-0:2.7.0";
const char TABLE CURRENT_KWH_TARIFF[]  = "0-0:96.14.0";
const char* const obis_table[] TABLE = {TOTAL_GAS_USED, CURRENT_USED_KW, CURRENT_RETURNED_KW, CURRENT_KWH_TARIFF};

/*
 /XMX5XMXABCE100105257

 0-0:96.1.1(4B414145303031343131373737353133)
 1-0:1.8.1(00441.491*kWh)
 1-0:1.8.2(00587.034*kWh)
 1-0:2.8.1(00000.000*kWh)
 1-0:2.8.2(00000.000*kWh)
 0-0:96.14.0(0002)
 1-0:1.7.0(0002.42*kW)
 1-0:2.7.0(0000.00*kW)
 0-0:17.0.0(999*A)
 0-0:96.3.10(1)
 0-0:96.13.1()
 0-0:96.13.0()
 0-1:96.1.0(3238303131303038333231313236383133)
 0-1:24.1.0(03)
 0-1:24.3.0(131223100000)(00)(60)(1)(0-1:24.2.0)(m3)
 (00490.415)
 0-1:24.4.0(1)
 ! 
*/
struct telegram_t 
{
  float gas_used_total;
  float kw_used_current;
  float kw_returned_current;
  uint8_t kw_tariff_current;
} PACKED;
