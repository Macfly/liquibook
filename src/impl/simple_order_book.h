#ifndef simple_order_book_h
#define simple_order_book_h

#include "liquibook_impl_export.h"
#include "simple_order.h"
#include "book/order_book.h"
#include "book/depth.h"

namespace liquibook { namespace impl {

class LIQUIBOOK_IMPL_Export SimpleOrderBook : 
      public book::OrderBook<SimpleOrder*> {
public:
  typedef book::Depth<5> SimpleDepth;
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

} }

#endif
