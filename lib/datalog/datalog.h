#ifndef DATALOG_H
#define DATALOG_H

/*
 * CSV line on the UART every LOG_PERIOD_S seconds (9600 8N1 by default),
 * only while FEAT_LOG is on:
 *
 *   GH,time,air_t,air_h,soil1,soil2,soil,lux,valve,pump,fan,heater,lamp,tank,day_l,fault
 *
 *   time   YYYY-MM-DD HH:MM:SS   (or "no-rtc")
 *   air_t  deg C, one decimal (24.5) | air_h %   ("-" when the sensor fails)
 *   soil1, soil2, soil  moisture %   (soil2 = "-" when not fitted)
 *   lux    light in lux ("-" when disabled / sensor does not answer)
 *   valve/pump/fan/heater/lamp  0|1
 *   tank   0 = ok, 1 = empty      day_l  litres used today
 *   fault  0 none, 1 no flow, 2 leak
 *
 * Lines starting with '#' are comments (a header is sent at start-up).
 */
void datalog_init(void);
void datalog_task(void);    /* call once per second */

#endif
