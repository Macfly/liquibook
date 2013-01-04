#ifndef types_h
#define types_h

#include <stdint.h>
#include "liquibook_export.h"

namespace liquibook {
  // Types used in Liquibook
  typedef uint32_t Price;
  typedef uint32_t Quantity;
  typedef uint32_t Cost;
  typedef uint32_t FillId;
  typedef uint32_t ChangeId;
  typedef uint32_t TransId;
  typedef uint32_t OrderConditions;

  enum OrderCondition {
    oc_all_or_none = 1,
    oc_immediate_or_cancel = oc_all_or_none * 2
  };

  // Constants used in liquibook
  extern const Price Liquibook_Export INVALID_LEVEL_PRICE;
  extern const Price Liquibook_Export MARKET_ORDER_PRICE;
  extern const Price Liquibook_Export MARKET_ORDER_BID_SORT_PRICE;
  extern const Price Liquibook_Export MARKET_ORDER_ASK_SORT_PRICE;
  extern const Price Liquibook_Export PRICE_UNCHANGED;

  extern const int32_t Liquibook_Export SIZE_UNCHANGED;
}

#endif
