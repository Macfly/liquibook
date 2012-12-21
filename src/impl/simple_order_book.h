#ifndef simple_order_book_h
#define simple_order_book_h

#include "liquibook_impl_export.h"
#include "simple_order.h"
#include "book/order_book.h"
#include "book/depth.h"
#include <iostream>

namespace liquibook { namespace impl {

template <int SIZE = 5>
class LIQUIBOOK_IMPL_Export SimpleOrderBook : 
      public book::OrderBook<SimpleOrder*> {
public:
  typedef typename book::Depth<SIZE> SimpleDepth;
  typedef book::Callback<SimpleOrder*> SimpleCallback;

  SimpleOrderBook();

  virtual void perform_callback(SimpleCallback& cb);
  const SimpleDepth& depth() const;

private:
  FillId fill_id_;
  SimpleDepth depth_;

  void restore_last_bid_level();
  void restore_last_ask_level();
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
        // Add to bid or ask depth
        if (cb.order_->is_buy()) {
          depth_.add_bid(cb.order_->price(), cb.order_->order_qty());
        } else {
          depth_.add_ask(cb.order_->price(), cb.order_->order_qty());
        }
      }
      break;

    case SimpleCallback::cb_order_fill:
      cb.order_->fill(cb.ref_qty_, cb.ref_cost_, ++fill_id_);
      // If the order is a limit order
      if (cb.order_->is_limit()) {
        // If this fill completed the order
        if (!cb.order_->open_qty()) {
          // If the filled order is a buy
          if (cb.order_->is_buy()) {
            if (depth_.close_bid(cb.order_->price(), cb.ref_qty_)) {
              restore_last_bid_level();
            }
          // Else the filled order is a sell
          } else {
            if (depth_.close_ask(cb.order_->price(), cb.ref_qty_)) {
              restore_last_ask_level();
            }
          }
        // Else this fill reduced the order
        } else {
          int32_t qty_delta = -cb.ref_qty_;
          // If the reduced order is a buy
          if (cb.order_->is_buy()) {
            depth_.change_qty_bid(cb.order_->price(), qty_delta);
          // Else the reduced order is a sell
          } else {
            depth_.change_qty_ask(cb.order_->price(), qty_delta);
          }
        }
      }
      break;

    case SimpleCallback::cb_order_cancel:
      cb.order_->cancel();
      // If the order is a limit order
      if (cb.order_->is_limit()) {
        // If the cancelled order is a buy
        if (cb.order_->is_buy()) {
          if (depth_.close_bid(cb.order_->price(), cb.order_->open_qty())) {
            restore_last_bid_level();
          }
        // Else the cancelled order is a sell
        } else {
          if (depth_.close_ask(cb.order_->price(), cb.order_->open_qty())) {
            restore_last_ask_level();
          }
        }
      }
      break;

    case SimpleCallback::cb_order_replace:
    {
      // Remember current values
      Price current_price = cb.order_->price();
      Quantity current_qty = cb.order_->open_qty();

      // Modify the order itself.  Do this first so restoration is accurate
      cb.order_->replace(cb.ref_qty_, cb.ref_price_);

      if (cb.order_->is_buy()) {
        if (depth_.replace_bid(current_price, cb.ref_price_, 
                               current_qty, cb.order_->open_qty())) {
          restore_last_bid_level();
        }
      } else {
        if (depth_.replace_ask(current_price, cb.ref_price_, 
                               current_qty, cb.order_->open_qty())) {
          restore_last_ask_level();
        }
      }


      break;
    }
    default:
      // Nothing
      break;
  }
}

template <int SIZE>
inline void
SimpleOrderBook<SIZE>::restore_last_bid_level()
{
  Price restoration_price;

  // If restoration is needed (meaning N-1 level is populated)
  if (depth_.needs_bid_restoration(restoration_price)) {
    // Restore the last remaining level
    populate_bid_depth_level_after(restoration_price, *depth_.last_bid_level());
  }
}

template <int SIZE>
inline void
SimpleOrderBook<SIZE>::restore_last_ask_level()
{
  Price restoration_price;

  // If restoration is needed (meaning N-1 level is populated)
  if (depth_.needs_ask_restoration(restoration_price)) {
    // Restore the last remaining level
    populate_ask_depth_level_after(restoration_price, *depth_.last_ask_level());
  }
}

template <int SIZE>
inline const typename SimpleOrderBook<SIZE>::SimpleDepth&
SimpleOrderBook<SIZE>::depth() const
{
  return depth_;
}

} }

#endif
