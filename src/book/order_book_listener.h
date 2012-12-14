#ifndef order_book_listener_h
#define order_book_listener_h

#include "order_book.h"

namespace liquibook { namespace book {

template <class OrderPtr = Order*>
class OrderBookListener
public:
  typedef OrderBook<OrderPtr> OrderBook;

  /// @brief callback for any book change
  virtual void on_book_change(OrderBook& book) = 0;

  /// @brief callback for change in aggregated depth
  virtual void on_depth_change(OrderBook& book) = 0;

  /// @brief callback for top of book change
  virtual void on_bbo_change(OrderBook& book) = 0;

};

} }

#endif
