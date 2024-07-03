#include <arch/i386/tsc.h>
#include <drivers/cmos.h>
#include <fs/devfs.h>
#include <interrupts.h>
#include <math.h>
#include <time.h>
#include <timer.h>
#include <typedefs.h>

int has_unix_time;
i64 start_unix_time;
u64 start_tsc_time;

void timer_start_init(void) {
  tsc_init();
  start_tsc_time = tsc_get();
  enable_interrupts();
  cmos_init();
  cmos_start_call(1, &has_unix_time, &start_unix_time);
  enable_interrupts();
}

void timer_wait_for_init(void) {
  enable_interrupts();
  for (; !has_unix_time;)
    ;
}

u64 timer_get_uptime(void) {
  return tsc_calculate_ms(tsc_get());
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
  (void)offset;
  u64 r = timer_get_ms();
  u64 l = min(len, sizeof(u64));
  memcpy(buffer, &r, l);
  return l;
}

int clock_write(u8 *buffer, u64 offset, u64 len, vfs_fd_t *fd) {
  (void)offset;
  if (len != sizeof(i64)) {
    return 0;
  }

  i64 new_value_ms;
  memcpy(&new_value_ms, buffer, sizeof(i64));
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
                 always_can_write, FS_TYPE_CHAR_DEVICE);
  return 1;
}
