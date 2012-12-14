#ifndef types_h
#define types_h

#include <stdint.h>
#include "liquibook_base_export.h"

namespace liquibook {
  typedef uint32_t Price;
  typedef uint32_t Quantity;
  typedef uint32_t Cost;
  typedef uint32_t FillId;

  extern const Price LIQUIBOOK_BASE_Export INVALID_LEVEL_PRICE;
  extern const Price LIQUIBOOK_BASE_Export MARKET_ORDER_PRICE;
  extern const Price LIQUIBOOK_BASE_Export MARKET_ORDER_BID_SORT_PRICE;
  extern const Price LIQUIBOOK_BASE_Export MARKET_ORDER_ASK_SORT_PRICE;
}


#endif
