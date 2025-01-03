#include <stdio.h>
#include <ubsan.h>

void ubsan_log(const char *cause, struct source_location source) {
  printf("UBSAN\n");
  printf("%s: %s : %d\n", cause, source.file_name, source.line);
  for (;;)
    ;
}

void __ubsan_handle_shift_out_of_bounds(struct ShiftOutOfBoundsData *data,
                                        unsigned long lhs, unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  ubsan_log("handle_shift_out_of_bounds", data->location);
}

void __ubsan_handle_add_overflow(struct OverflowData *data, unsigned long lhs,
                                 unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  ubsan_log("handle_add_overflow", data->location);
}

void __ubsan_handle_sub_overflow(struct OverflowData *data, unsigned long lhs,
                                 unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  ubsan_log("handle_sub_overflow", data->location);
}

void __ubsan_handle_negate_overflow(struct OverflowData *data,
                                    unsigned long lhs, unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  ubsan_log("handle_negate_overflow", data->location);
}

void __ubsan_handle_mul_overflow(struct OverflowData *data, unsigned long lhs,
                                 unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  ubsan_log("handle_mul_overflow", data->location);
}

void __ubsan_handle_pointer_overflow(struct OverflowData *data, unsigned long lhs,
                                 unsigned long rhs) {
  (void)lhs;
  (void)rhs;
  ubsan_log("handle_poinetr_overflow", data->location);
}

void __ubsan_handle_out_of_bounds(struct OutOfBoundsData *data, void *index) {
  (void)index;
  ubsan_log("handle_out_of_bounds", data->location);
}

void __ubsan_handle_vla_bound_not_positive(struct OutOfBoundsData *data,
                                           void *index) {
  (void)index;
  ubsan_log("handle_vla_bound_not_positive", data->location);
}
