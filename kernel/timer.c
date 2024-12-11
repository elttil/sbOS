#include <arch/i386/tsc.h>
#include <drivers/cmos.h>
#include <fs/devfs.h>
#include <interrupts.h>
#include <lib/sv.h>
#include <math.h>
#include <random.h>
#include <time.h>
#include <timer.h>
#include <typedefs.h>

int has_unix_time;
i64 start_unix_time;
u64 start_tsc_time;

void timer_start_init(void) {
  tsc_init();
  start_tsc_time = tsc_get();
  random_add_entropy((u8 *)&start_tsc_time, sizeof(start_tsc_time));
  enable_interrupts();
  cmos_init();
  cmos_start_call(1, &has_unix_time, &start_unix_time);
}

void timer_wait_for_init(void) {
  enable_interrupts();
  for (; !has_unix_time;)
    ;
}

u64 timer_current_uptime = 0; // This gets updated by the PIT handler
u64 timer_get_uptime(void) {
  return timer_current_uptime;
}

void timer_get(struct timespec *tp) {
  u64 offset_tsc = tsc_get() - start_tsc_time;
  i64 current_unix_time_seconds =
      start_unix_time + tsc_calculate_ms(offset_tsc) / 1000;
  tp->tv_sec = current_unix_time_seconds;
  tp->tv_nsec = tsc_calculate_ms(offset_tsc);
}

u64 timer_get_ms(void) {
  u64 offset_tsc = tsc_get() - start_tsc_time;
  return start_unix_time * 1000 + tsc_calculate_ms(offset_tsc);
}

int clock_read(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  if (0 != offset) {
    return 0;
  }
  u64 r = timer_get_ms();
  return min(len, (u64)kbnprintf(buffer, len, "%llu", r));
}

int clock_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  if (0 != offset) {
    return 0;
  }

  struct sv string_view = sv_init(buffer, len);

  u64 new_value_ms = sv_parse_unsigned_number(string_view, &string_view, NULL);

  i64 new_value_seconds = new_value_ms / 1000;

  u64 offset_tsc = tsc_get() - start_tsc_time;
  i64 current_unix_time_seconds =
      start_unix_time + tsc_calculate_ms(offset_tsc) / 1000;

  i64 delta = new_value_seconds - current_unix_time_seconds;
  start_unix_time += delta;
  int done;
  enable_interrupts();
  cmos_start_call(0, &done, &new_value_seconds);
  for (; !done;)
    ;
  return sizeof(i64);
}

int always_has_data(vfs_inode_t *inode);
int always_can_write(vfs_inode_t *inode);

int timer_add_clock(void) {
  devfs_add_file("/clock", clock_read, clock_write, NULL, always_has_data,
                 always_can_write, FS_TYPE_BLOCK_DEVICE);
  return 1;
}
