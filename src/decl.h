#include <wchar.h>

typedef struct macro_payload macro_payload_t;
int macro_destroy_payload(macro_payload_t*);
macro_payload_t *macro_create_payload();

typedef struct io_interface_s io_interface_t;
void io_put(io_interface_t*, wchar_t);
