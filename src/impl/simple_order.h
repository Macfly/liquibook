#ifndef simpleorder_h
#define simpleorder_h

#include "liquibook_impl_export.h"
#include "book/order.h"
#include "base/types.h"

namespace liquibook { namespace impl {

enum OrderState {
  os_new,
  os_accepted,
  os_complete,
  os_cancelled,
  os_rejected
};

class LIQUIBOOK_IMPL_Export SimpleOrder : public book::Order {
public:
  SimpleOrder(bool is_buy,
              Price price,
              Quantity qty);

  /// @brief get the order's state
  const OrderState& state() const;

  /// @brief is this order a buy?
  virtual bool is_buy() const;

  /// @brief get the limit price of this order
  virtual Price price() const;

  /// @brief get the quantity of this order
  virtual Quantity order_qty() const;

  /// @brief get the open quantity of this order
  virtual Quantity open_qty() const;

  /// @brief get the filled quantity of this order
  virtual const Quantity& filled_qty() const;

  /// @brief get the total filled cost of this order
  const Cost& filled_cost() const;

  /// @brief notify of a fill of this order
  /// @param fill_qty the number of shares in this fill
  /// @param fill_cost the total amount of this fill
  /// @fill_id the unique identifier of this fill
  virtual void fill(Quantity fill_qty, 
                    Cost fill_cost,
                    FillId fill_id);

  /// @brief exchange accepted this order
  void accept();
  /// @brief exchange cancelled this order
  void cancel();

  /// @brief exchange replaced this order
  /// @param new_order_qty the new order quantity
  /// @param new_price the new price
  void replace(Quantity new_order_qty, Price new_price);

private:
  OrderState state_;
  bool is_buy_;
  Price    price_;
  Quantity order_qty_;
  Quantity filled_qty_;
  Cost filled_cost_;
  static uint32_t last_order_id_;

public:
  const uint32_t order_id_;
};

} }

#endif
