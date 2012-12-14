#ifndef depth_level_h
#define depth_level_h

#include "liquibook_book_export.h"
#include "base/types.h"

namespace liquibook { namespace book {

class LIQUIBOOK_BOOK_Export DepthLevel {
public:
  DepthLevel();
  DepthLevel(Price price,
             uint32_t order_count,
             Quantity aggregate_qty);

  const Price& price() const;
  uint32_t order_count() const;
  Quantity aggregate_qty() const;

  void init(Price price);

  /// @brief add an order to the level
  /// @param qty open quantity of the order
  void add_order(Quantity qty);

  /// @brief increase the quantity of existing orders
  /// @param qty amount to increase the quantity by
  void increase_qty(Quantity qty);

  /// @brief decrease the quantity of existing orders
  /// @param qty amount to decrease the quantity by
  void decrease_qty(Quantity qty);

  /// @brief cancel or fill an order, decrease count and quantity
  /// @param qty the closed quantity
  /// @return true if the level is now empty
  bool close_order(Quantity qty);

private:
  Price price_;
  uint32_t order_count_;
  Quantity aggregate_qty_;
};

} }

#endif
