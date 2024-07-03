#include <assert.h>
#include <cpu/idt.h>
#include <cpu/io.h>
#include <drivers/cmos.h>

/*
 * Why the fuck would somebody go through the effort of converting their
 * seconds counter to minutes, hours IN PM AND AM AND THE DAYS OF THE MONTH???
 * It is obvious that the first thing any person reading these values
 * will do is convert them into seconds SINCE THAT IS THE ONLY SANE WAY
 * OF STORING TIME FOR COMPUTERS.
 *
 * Register  Contents            Range
 * 0x00      Seconds             0–59
 * 0x02      Minutes             0–59
 * 0x04      Hours               0–23 in 24-hour mode,
 *                               1–12 in 12-hour mode, highest bit set if pm
 * 0x06      Weekday             1–7, Sunday = 1
 * 0x07      Day of Month        1–31
 * 0x08      Month               1–12
 * 0x09      Year                0–99
 * 0x32      Century (maybe)     19–20?
 * 0x0A      Status Register A
 * 0x0B      Status Register B
 */
#define CMOS_SECONDS 0x00
#define CMOS_MINUTES 0x02
#define CMOS_HOURS 0x04
#define CMOS_WEEKDAY 0x06
#define CMOS_DAY_OF_MONTH 0x07
#define CMOS_MONTH 0x08
#define CMOS_YEAR 0x09
#define CMOS_CENTURY 0x32
#define CMOS_REG_B 0x0B

#define NMI_disable_bit 1

static u8 cmos_get_register(u8 reg) {
  outb(0x70, (NMI_disable_bit << 7) | reg);
  return inb(0x71);
}

static void cmos_set_register(u8 reg, u8 value) {
  outb(0x70, (NMI_disable_bit << 7) | reg);
  outb(0x71, value);
}

static u8 binary_to_bcd(int binary) {
  int result = 0;
  int shift = 0;
  while (binary > 0) {
    result |= (binary % 10) << (shift++ << 2);
    binary /= 10;
  }
  return result;
}

static u8 bcd_to_binary(u8 bcd) {
  return ((bcd / 16) * 10) + (bcd & 0xf);
}

u8 days_in_month[] = {
    31, // January: 31 days
    28, // February: 28 days and 29 in every leap year
    31, // March: 31 days
    30, // April: 30 days
    31, // May: 31 days
    30, // June: 30 days
    31, // July: 31 days
    31, // August: 31 days
    30, // September: 30 days
    31, // October: 31 days
    30, // November: 30 days
    31, // December: 31 days
};

static i64 cmos_get_time(void);
static void cmos_set_time(i64 time);

int cmos_has_command = 0;
int cmos_command_is_read = 0;
int *cmos_done_ptr = NULL;
i64 *cmos_time_ptr = NULL;

void cmos_handler(reg_t *reg) {
  (void)reg;
  if (!cmos_has_command) {
    goto cmos_handler_exit;
  }

  if (cmos_command_is_read) {
    *cmos_time_ptr = cmos_get_time();
    *cmos_done_ptr = 1;
    goto cmos_handler_exit;
  }

  if (!cmos_command_is_read) {
    cmos_set_time(*cmos_time_ptr);
    *cmos_done_ptr = 1;
    goto cmos_handler_exit;
  }

cmos_handler_exit:
  cmos_has_command = 0;
  u8 reg_b = cmos_get_register(CMOS_REG_B);
  reg_b &= ~0x40; // Disable interrupts
  cmos_set_register(CMOS_REG_B, reg_b);
  EOI(0x8);
}

void cmos_init(void) {
  install_handler((interrupt_handler)cmos_handler, INT_32_INTERRUPT_GATE(0x0),
                  0x28);
}

int cmos_start_call(int is_read, int *done, i64 *time) {
  if (cmos_has_command) {
    return 0;
  }
  *done = 0;
  cmos_done_ptr = done;
  cmos_time_ptr = time;
  cmos_command_is_read = is_read;
  cmos_has_command = 1;
  u8 reg_b = cmos_get_register(CMOS_REG_B);
  reg_b |= 0x40; // Enable interrupts
  cmos_set_register(CMOS_REG_B, reg_b);
  return 1;
}

// This function returns the current unix timestamp from the CMOS RTC
// TODO: It currently makes the assumption that time travel is not possible and
// as a result will not give negative values. This support should maybe be added
// to prepare ourselves for the past.
static i64 cmos_get_time(void) {
  u8 seconds = cmos_get_register(CMOS_SECONDS);
  u8 minutes = cmos_get_register(CMOS_MINUTES);

  u8 reg_b = cmos_get_register(0x0B);

  u8 hours = cmos_get_register(CMOS_HOURS);
  u8 day_of_month = cmos_get_register(CMOS_DAY_OF_MONTH);
  u8 month = cmos_get_register(CMOS_MONTH);
  u8 incomplete_year = cmos_get_register(CMOS_YEAR);
  u8 century = cmos_get_register(CMOS_CENTURY);
  if (!(reg_b & (1 << 2))) {
    hours = bcd_to_binary(hours);
    day_of_month = bcd_to_binary(day_of_month);
    month = bcd_to_binary(month);
    incomplete_year = bcd_to_binary(incomplete_year);
    century = bcd_to_binary(century);
  }
  if (!(reg_b & (1 << 1))) {
    // TODO: Add support for AM/PM so the americans can use this
    return 0;
  }

  u16 year = century * 100 + incomplete_year;

  u64 second_sum = 0;

  // TODO: There is probably a more clever algorithm for this
  const u32 seconds_in_a_day = 24 * 60 * 60;
  for (int i = 1970; i < year; i++) {
    second_sum += 365 * seconds_in_a_day;
    if (0 == i % 4) {
      second_sum += seconds_in_a_day;
    }
  }

  for (int i = 0; i < (int)month - 1
       /*-1 to account for it being 1 indexed*/;
       i++) {
    second_sum += days_in_month[i] * seconds_in_a_day;
    int is_leap_year = 0 == (year % 4);
    if (is_leap_year && 1 == i /*February*/) {
      second_sum += seconds_in_a_day;
    }
  }
  second_sum += (day_of_month - 1) * seconds_in_a_day;
  second_sum += hours * 60 * 60;
  second_sum += minutes * 60;
  second_sum += seconds;
  return second_sum;
}

static void cmos_set_time(i64 time) {
  assert(time > 0); // TODO: Add support for time travelers
  u8 reg_b = cmos_get_register(0x0B);

  u8 seconds = 0;
  u8 minutes = 0;
  u8 hours = 0;

  const u32 seconds_in_a_day = 24 * 60 * 60;

  // 1 January 1970 was a thursday, that is why there is a +4
  u8 weekday = ((((time / seconds_in_a_day) + 4)) % 7) + 1;
  u8 day_of_month = 0;
  u8 month = 0;
  u16 year = 1970;

  if (!(reg_b & (1 << 1))) {
    // TODO: Add support for AM/PM so the americans can use this
    return;
  }

  // Keep adding years for as long as possible
  for (;;) {
    u32 s = 365 * seconds_in_a_day;
    if (0 == year % 4) {
      s += seconds_in_a_day;
      if (0 == year % 100 && 0 != year % 400) {
        s -= seconds_in_a_day;
      }
    }
    if (s > time) {
      break;
    }
    year++;
    time -= s;
  }
  // Keep adding months
  for (;;) {
    u32 s = days_in_month[month] * seconds_in_a_day;
    if (0 == (year % 4) && 1 == month && (0 != year % 100 || 0 == year % 400)) {
      s += seconds_in_a_day;
    }
    if (s > time) {
      break;
    }
    month++;
    time -= s;
  }
  month++;
  day_of_month = time / seconds_in_a_day;
  day_of_month++;
  time %= seconds_in_a_day;

  hours = time / (60 * 60);
  time %= 60 * 60;

  minutes = time / (60);
  time %= 60;

  seconds = time;

  u8 s_year = year % 100;
  u8 century = (year - (year % 100)) / 100;
  if (!(reg_b & (1 << 2))) {
    seconds = binary_to_bcd(seconds);
    minutes = binary_to_bcd(minutes);
    hours = binary_to_bcd(hours);
    day_of_month = binary_to_bcd(day_of_month);
    month = binary_to_bcd(month);
    weekday = binary_to_bcd(weekday);
    s_year = binary_to_bcd(s_year);
    century = binary_to_bcd(century);
  }
  cmos_set_register(CMOS_SECONDS, seconds);
  cmos_set_register(CMOS_MINUTES, minutes);
  cmos_set_register(CMOS_HOURS, hours);
  cmos_set_register(CMOS_DAY_OF_MONTH, day_of_month);
  cmos_set_register(CMOS_MONTH, month);
  cmos_set_register(CMOS_YEAR, s_year);
  cmos_set_register(CMOS_CENTURY, century);
  cmos_set_register(CMOS_WEEKDAY, weekday);
}
