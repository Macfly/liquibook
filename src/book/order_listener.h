#ifndef order_listener_h
#define order_listener_h

#include "order.h"

namespace liquibook { namespace book {

template <class OrderPtr = Order*>
class OrderListener {
public:
  /// @brief callback for an order accept
  virtual void on_accept(const OrderPtr& order) = 0;

  /// @brief callback for an order reject
  virtual void on_reject(const OrderPtr& order, const char* reason) = 0;

  /// @brief callback for an order fill
  virtual void on_fill(const OrderPtr& order, Quantity fill_qty, Cost cost) = 0;

  /// @brief callback for an order cancellation
  virtual void on_cancel(const OrderPtr& order) = 0;

  /// @brief callback for an order cancel rejection
  virtual void on_cancel_reject(const OrderPtr& order, const char* reason) = 0;

  /// @brief callback for an order replace
  virtual void on_replace(const OrderPtr& order) = 0;

  /// @brief callback for an order replace rejection
  virtual void on_replace_reject(const OrderPtr& order, const char* reason) = 0;
};

} }

#endif
