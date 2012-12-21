#ifndef callback_h
#define callback_h

#include "order.h"
#include "base/types.h"

namespace liquibook { namespace book {

template <class OrderPtr>
class OrderBook;

// Callback events
//   New order accept
//     - order accept
//     - fill (2) and/or quote (if not complete)
//     - depth/bbo ?
//   New order reject
//     - order reject
//   Order fill
//     - fill (2)
//     - trade
//     - quote (2)
//     - depth/bbo ?
//   Order cancell
//     - order cancel
//     - quote
//     - depth/bbo ?
//   Order cancel reject
//     - order cancel reject
//   Order replace
//     - order replace
//     - fill (2) and/or quote (if not complete)
//     - depth/bbo ?
//   Order replace reject
//     - order replace reject

template <class OrderPtr = Order*>
class Callback {
public:
  typedef OrderBook<OrderPtr > TypedOrderBook;

  enum CbType {
    cb_unknown,
    cb_order_accept,
    cb_order_reject,
    cb_order_fill,
    cb_order_cancel,
    cb_order_cancel_reject,
    cb_order_replace,
    cb_order_replace_reject,
    cb_book_update,
    cb_depth_update,
    cb_bbo_update
  };

  Callback();

  static Callback<OrderPtr> accept(const OrderPtr& order);
  static Callback<OrderPtr> reject(const OrderPtr& order,
                                   const char* reason);
  static Callback<OrderPtr> fill(const OrderPtr& order,
                                 const Quantity& qty,
                                 const Price& price,
                                 const Cost& cost,
                                 const TransId& transactionId);
  static Callback<OrderPtr> cancel(const OrderPtr& order);
  static Callback<OrderPtr> cancel_reject(const OrderPtr& order,
                                          const char* reason);
  static Callback<OrderPtr> replace(const OrderPtr& order,
                                    const Quantity& new_order_qty,
                                    const Price& new_price);
  static Callback<OrderPtr> replace_reject(const OrderPtr& order,
                                           const char* reason);

  CbType type_;
  OrderPtr order_;
  const char* reject_reason_;
  Quantity ref_qty_;
  Price ref_price_;
  Cost ref_cost_;
  TransId ref_id_;
};

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::accept(const OrderPtr& order)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_accept;
  result.order_ = order;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::reject(
  const OrderPtr& order,
  const char* reason)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_reject;
  result.order_ = order;
  result.reject_reason_ = reason;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::fill(const OrderPtr& order,
                                            const Quantity& qty,
                                            const Price& price,
                                            const Cost& cost,
                                            const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_fill;
  result.order_ = order;
  result.ref_qty_ = qty;
  result.ref_price_ = price;
  result.ref_cost_ = cost;
  result.ref_id_ = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::cancel(const OrderPtr& order)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_cancel;
  result.order_ = order;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::cancel_reject(
  const OrderPtr& order,
  const char* reason)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_cancel_reject;
  result.order_ = order;
  result.reject_reason_ = reason;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr>::Callback()
: type_(cb_unknown),
  reject_reason_(NULL),
  ref_qty_(0),
  ref_price_(0),
  ref_cost_(0),
  ref_id_(0)
{
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::replace(
  const OrderPtr& order,
  const Quantity& new_order_qty,
  const Price& new_price)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_replace;
  result.order_ = order;
  result.ref_qty_ = new_order_qty;
  result.ref_price_ = new_price;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::replace_reject(
  const OrderPtr& order,
  const char* reason)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_replace_reject;
  result.order_ = order;
  result.reject_reason_ = reason;
  return result;
}

} }

#endif
