#ifndef simple_order_book_h
#define simple_order_book_h

#include "liquibook_impl_export.h"
#include "simple_order.h"
#include "book/order_book.h"
#include "book/depth.h"
#include <iostream>

namespace liquibook { namespace impl {

template <int SIZE = 5>
class SimpleOrderBook : 
      public book::OrderBook<SimpleOrder*> {
public:
  typedef typename book::Depth<SIZE> SimpleDepth;
  typedef book::Callback<SimpleOrder*> SimpleCallback;

  SimpleOrderBook();

  virtual void perform_callback(SimpleCallback& cb);
  SimpleDepth& depth();
  const SimpleDepth& depth() const;

private:
  FillId fill_id_;
  SimpleDepth depth_;
};


template <int SIZE>
SimpleOrderBook<SIZE>::SimpleOrderBook()
: fill_id_(0)
{
}

template <int SIZE>
inline void
SimpleOrderBook<SIZE>::perform_callback(SimpleCallback& cb)
{
  switch(cb.type_) {
    case SimpleCallback::cb_order_accept:
      cb.order_->accept();
      // If the order is a limit order
      if (cb.order_->is_limit()) {
        // If the order is completely filled on acceptance, do not modify 
        // depth unnecessarily
        if (cb.ref_qty_ == cb.order_->order_qty()) {
          // Don't tell depth about this order - it's going away immediately.
          // Instead tell Depth about future fills to ignore
          depth_.ignore_fill_qty(cb.ref_qty_, cb.order_->is_buy());
        } else {
          // Add to bid or ask depth
          depth_.add_order(cb.order_->price(), 
                           cb.order_->order_qty(), 
                           cb.order_->is_buy());
        }
      }
      break;

    case SimpleCallback::cb_order_fill:
      // If the matched order is a limit order
      if (cb.matched_order_->is_limit()) {
        // Inform the depth
        depth_.fill_order(cb.matched_order_->price(), 
                          cb.matched_order_->open_qty(),
                          cb.ref_qty_,
                          cb.matched_order_->is_buy());
      }
      // If the inbound order is a limit order
      if (cb.order_->is_limit()) {
        // Inform the depth
        depth_.fill_order(cb.order_->price(), 
                          cb.order_->open_qty(),
                          cb.ref_qty_,
                          cb.order_->is_buy());
      }
      // Increment fill ID once
      ++fill_id_;
      // Update the orders
      cb.matched_order_->fill(cb.ref_qty_, cb.ref_cost_, fill_id_);
      cb.order_->fill(cb.ref_qty_, cb.ref_cost_, fill_id_);
      break;

    case SimpleCallback::cb_order_cancel:
      cb.order_->cancel();
      // If the order is a limit order
      if (cb.order_->is_limit()) {
        // If the close erases a level
        depth_.close_order(cb.order_->price(), 
                           cb.order_->open_qty(), 
                           cb.order_->is_buy());
      }
      break;

    case SimpleCallback::cb_order_replace:
    {
      // Remember current values
      Price current_price = cb.order_->price();
      Quantity current_qty = cb.order_->open_qty();

      // Modify the order itself
      cb.order_->replace(cb.ref_qty_, cb.ref_price_);

      // Notify the depth
      depth_.replace_order(current_price, cb.ref_price_, 
                           current_qty, cb.order_->open_qty(),
                           cb.order_->is_buy());
      break;
    }
    default:
      // Nothing
      break;
  }
}

template <int SIZE>
inline typename SimpleOrderBook<SIZE>::SimpleDepth&
SimpleOrderBook<SIZE>::depth()
{
  return depth_;
}

template <int SIZE>
inline const typename SimpleOrderBook<SIZE>::SimpleDepth&
SimpleOrderBook<SIZE>::depth() const
{
  return depth_;
}

} }

#endif
