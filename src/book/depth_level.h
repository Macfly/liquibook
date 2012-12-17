#ifndef depth_level_h
#define depth_level_h

#include "liquibook_book_export.h"
#include "base/types.h"

namespace liquibook { namespace book {

class LIQUIBOOK_BOOK_Export DepthLevel {
public:
  /// @brief construct
  DepthLevel();

  /// @brief assign
  DepthLevel& operator=(const DepthLevel& rhs);

  /// @brief get price
  const Price& price() const;
  /// @brief get count
  uint32_t order_count() const;
  /// @brief get aggregate quantity
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

  void last_change(ChangeId last_change) { last_change_ = last_change; }
  ChangeId last_change() const { return last_change_; }
  bool changed_since(ChangeId last_published_change) const;

private:
  Price price_;
  uint32_t order_count_;
  Quantity aggregate_qty_;
public:
  ChangeId last_change_;
};

inline bool
DepthLevel::changed_since(ChangeId last_published_change) const
{
  return last_change_ > last_published_change;
}

} }

#endif
