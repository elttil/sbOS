#include <stdint.h>

enum { type_kind_int = 0, type_kind_float = 1, type_unknown = 0xffff };

struct type_descriptor {
  uint16_t type_kind;
  uint16_t type_info;
  char type_name[1];
};

struct source_location {
  const char *file_name;
  union {
    unsigned long reported;
    struct {
      uint32_t line;
      uint32_t column;
    };
  };
};

struct OverflowData {
  struct source_location location;
  struct type_descriptor *type;
};

struct type_mismatch_data {
  struct source_location location;
  struct type_descriptor *type;
  unsigned long alignment;
  unsigned char type_check_kind;
};

struct type_mismatch_data_v1 {
  struct source_location location;
  struct type_descriptor *type;
  unsigned char log_alignment;
  unsigned char type_check_kind;
};

struct type_mismatch_data_common {
  struct source_location *location;
  struct type_descriptor *type;
  unsigned long alignment;
  unsigned char type_check_kind;
};

struct nonnull_arg_data {
  struct source_location location;
  struct source_location attr_location;
  int arg_index;
};

struct OutOfBoundsData {
  struct source_location location;
  struct type_descriptor *array_type;
  struct type_descriptor *index_type;
};

struct ShiftOutOfBoundsData {
  struct source_location location;
  struct type_descriptor *lhs_type;
  struct type_descriptor *rhs_type;
};

struct unreachable_data {
  struct source_location location;
};

struct invalid_value_data {
  struct source_location location;
  struct type_descriptor *type;
};

struct alignment_assumption_data {
  struct source_location location;
  struct source_location assumption_location;
  struct type_descriptor *type;
};
