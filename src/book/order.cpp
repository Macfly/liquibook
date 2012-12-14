#include "order.h"

namespace liquibook { namespace book {
bool
Order::is_limit() const {
  return (price() > 0);
}

} }
