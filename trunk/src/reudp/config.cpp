#include "config.h"

namespace reudp {
namespace config {
    obj _obj;

    obj::obj() : timeout(2), send_try_count(3) {}
}
}
