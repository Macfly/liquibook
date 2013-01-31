// Copyright (c) 2012, 2013 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#ifndef callback_h
#define callback_h

#include "order.h"
#include "types.h"

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
//   Order cancel
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

/// @brief notification from OrderBook of an event
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
    cb_depth_update,
    cb_bbo_update
  };

  Callback();

  /// @brief create a new accept callback
  static Callback<OrderPtr> accept(const OrderPtr& order,
                                   const TransId& trans_id);
  /// @brief create a new reject callback
  static Callback<OrderPtr> reject(const OrderPtr& order,
                                   const char* reason,
                                   const TransId& trans_id);
  /// @brief create a new fill callback
  static Callback<OrderPtr> fill(const OrderPtr& inbound_order,
                                 const OrderPtr& matched_order,
                                 const Quantity& qty,
                                 const Price& price,
                                 const TransId& trans_id);
  /// @brief create a new cancel callback
  static Callback<OrderPtr> cancel(const OrderPtr& order,
                                   const TransId& trans_id);
  /// @brief create a new cancel reject callback
  static Callback<OrderPtr> cancel_reject(const OrderPtr& order,
                                          const char* reason,
                                          const TransId& trans_id);
  /// @brief create a new replace callback
  static Callback<OrderPtr> replace(const OrderPtr& order,
                                    const Quantity& new_order_qty,
                                    const Price& new_price,
                                    const TransId& trans_id);
  /// @brief create a new replace reject callback
  static Callback<OrderPtr> replace_reject(const OrderPtr& order,
                                           const char* reason,
                                           const TransId& trans_id);

  CbType type;
  OrderPtr order;
  OrderPtr matched_order; // fill
  TransId trans_id;
  union {
    struct {
      Quantity match_qty;
    };
    struct {
      Quantity fill_qty;
      Price fill_price;
    };
    struct {
      Quantity new_order_qty;
      Price new_price;
    };
    const char* reject_reason;
  };
};

template <class OrderPtr>
Callback<OrderPtr>::Callback()
: type(cb_unknown),
  trans_id(0),
  fill_qty(0),
  fill_price(0)
{
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::accept(
  const OrderPtr& order,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type = cb_order_accept;
  result.order = order;
  result.trans_id = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::reject(
  const OrderPtr& order,
  const char* reason,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type = cb_order_reject;
  result.order = order;
  result.reject_reason = reason;
  result.trans_id = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::fill(
  const OrderPtr& inbound_order,
  const OrderPtr& matched_order,
  const Quantity& qty,
  const Price& price,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type = cb_order_fill;
  result.order = inbound_order;
  result.matched_order = matched_order;
  result.fill_qty = qty;
  result.fill_price = price;
  result.trans_id = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::cancel(
  const OrderPtr& order,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type = cb_order_cancel;
  result.order = order;
  result.trans_id = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::cancel_reject(
  const OrderPtr& order,
  const char* reason,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type = cb_order_cancel_reject;
  result.order = order;
  result.reject_reason = reason;
  result.trans_id = trans_id;
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
  result.type = cb_order_replace;
  result.order = order;
  result.new_order_qty = new_order_qty;
  result.new_price = new_price;
  result.trans_id = trans_id;
  return result;
}

template <class OrderPtr>
Callback<OrderPtr> Callback<OrderPtr>::replace_reject(
  const OrderPtr& order,
  const char* reason,
  const TransId& trans_id)
{
  Callback<OrderPtr> result;
  result.type = cb_order_replace_reject;
  result.order = order;
  result.reject_reason = reason;
  result.trans_id = trans_id;
  return result;
}

} }

#endif
