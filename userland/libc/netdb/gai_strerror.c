#include <assert.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>

const char *gai_strerror(int errcode) {
  switch (errcode) {
  case EAI_AGAIN:
    return "The name could not be resolved at this time. Future attempts may "
           "succeed.";
  case EAI_BADFLAGS:
    return "The flags had an invalid value.";
  case EAI_FAIL:
    return "A non-recoverable error occurred.";
  case EAI_FAMILY:
    return "The address family was not recognized or the address length was "
           "invalid for the specified family.";
  case EAI_MEMORY:
    return "There was a memory allocation failure.";
  case EAI_NONAME:
    return "The name does not resolve for the supplied parameters. NI_NAMEREQD "
           "is set and the host's name cannot be located, or both nodename and "
           "servname were null.";
  case EAI_SERVICE:
    return "The service passed was not recognized for the specified socket "
           "type.";
  case EAI_SOCKTYPE:
    return "The intended socket type was not recognized.";
  case EAI_SYSTEM:
    return "A system error occurred. The error code can be found in errno.";
  case EAI_OVERFLOW:
    return "An argument buffer overflowed.";
  default:
    return "Invalid gai error code, no error string found.";
  }
}
