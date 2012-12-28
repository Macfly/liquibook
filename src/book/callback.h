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

  static Callback<OrderPtr> accept(const OrderPtr& order,
                                   const TransId& trans_id);
  static Callback<OrderPtr> reject(const OrderPtr& order,
                                   const char* reason,
                                   const TransId& trans_id);
  static Callback<OrderPtr> fill(const OrderPtr& inbound_order,
                                 const OrderPtr& matched_order,
                                 const Quantity& qty,
                                 const Price& price,
                                 const Cost& cost,
                                 const TransId& trans_id);
  static Callback<OrderPtr> cancel(const OrderPtr& order,
                                   const TransId& trans_id);
  static Callback<OrderPtr> cancel_reject(const OrderPtr& order,
                                          const char* reason,
                                          const TransId& trans_id);
  static Callback<OrderPtr> replace(const OrderPtr& order,
                                    const Quantity& new_order_qty,
                                    const Price& new_price,
                                    const TransId& trans_id);
  static Callback<OrderPtr> replace_reject(const OrderPtr& order,
                                           const char* reason,
                                           const TransId& trans_id);

  CbType type_;
  OrderPtr order_;
  OrderPtr matched_order_;
  const char* reject_reason_;
  Quantity ref_qty_;
  Price ref_price_;
  Cost ref_cost_;
  TransId trans_id_;
};

template <class OrderPtr>
Callback<OrderPtr>::Callback()
: type_(cb_unknown),
  reject_reason_(NULL),
  ref_qty_(0),
  ref_price_(0),
  ref_cost_(0),
  trans_id_(0)
{
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::accept(
  const OrderPtr& order,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_accept;
  result.order_ = order;
  result.trans_id_ = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::reject(
  const OrderPtr& order,
  const char* reason,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_reject;
  result.order_ = order;
  result.reject_reason_ = reason;
  result.trans_id_ = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::fill(
  const OrderPtr& inbound_order,
  const OrderPtr& matched_order,
  const Quantity& qty,
  const Price& price,
  const Cost& cost,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_fill;
  result.order_ = inbound_order;
  result.matched_order_ = matched_order;
  result.ref_qty_ = qty;
  result.ref_price_ = price;
  result.ref_cost_ = cost;
  result.trans_id_ = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::cancel(
  const OrderPtr& order,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_cancel;
  result.order_ = order;
  result.trans_id_ = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::cancel_reject(
  const OrderPtr& order,
  const char* reason,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_cancel_reject;
  result.order_ = order;
  result.reject_reason_ = reason;
  result.trans_id_ = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::replace(
  const OrderPtr& order,
  const Quantity& new_order_qty,
  const Price& new_price,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_replace;
  result.order_ = order;
  result.ref_qty_ = new_order_qty;
  result.ref_price_ = new_price;
  result.trans_id_ = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::replace_reject(
  const OrderPtr& order,
  const char* reason,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type_ = cb_order_replace_reject;
  result.order_ = order;
  result.reject_reason_ = reason;
  result.trans_id_ = trans_id;
  return result;
}

} }

#endif
