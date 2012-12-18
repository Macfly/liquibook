#ifndef order_book_h
#define order_book_h

#include "liquibook_book_export.h"
#include "callback.h"
#include "order.h"
#include "order_listener.h"
#include "depth_level.h"
#include <map>
#include <deque>
#include <iostream>
#include <stdexcept>
#include <cmath>

namespace liquibook { namespace book {

class OrderBookListener;

template<class OrderPtr>
class OrderListener;

template <class OrderPtr = Order*>
class OrderTracker {
public:
  OrderTracker(const OrderPtr& order);
  void fill(Quantity qty); 

  bool filled() const;
  Quantity open_qty() const;

  const OrderPtr& ptr() const;
  OrderPtr& ptr();
private:
  OrderPtr order_;
  Quantity filled_qty_;
};

template <class OrderPtr = Order*>
class LIQUIBOOK_BOOK_Export OrderBook {
public:
  typedef OrderTracker<OrderPtr > Tracker;
  typedef Callback<OrderPtr > TypedCallback;
  typedef OrderListener<OrderPtr > TypedOrderListener;
  typedef std::deque<TypedCallback > Callbacks;
  typedef std::multimap<Price, Tracker, std::greater<Price> >  Bids;
  typedef std::multimap<Price, Tracker, std::less<Price> >     Asks;

  /// @brief construct
  OrderBook();

  /// @brief add an order to book
  /// @return true if the add resulted in a fill
  virtual bool add(const OrderPtr& order);

  /// @brief cancel an order in the book
  virtual void cancel(const OrderPtr& order);

  /// @brief replace an order in the book
  /// @param order the order to replace
  /// @param size_delta the change in size for the order (positive or negative)
  /// @param new_price the new order price, or PRICE_UNCHANGED
  /// @return true if the replace resulted in a fill
  virtual bool replace(const OrderPtr& order, 
                       int32_t size_delta = SIZE_UNCHANGED,
                       Price new_price = PRICE_UNCHANGED);

  /// @brief access the bids container
  const Bids& bids() const { return bids_; };

  /// @brief access the asks container
  const Asks& asks() const { return asks_; };

  /// @brief perform all callbacks in the queue
  virtual void perform_callbacks();

  /// @brief perform an individual callback
  virtual void perform_callback(TypedCallback& cb);

  void log() const;

  /// @brief populate a bid depth level after a given price, to rebuild a level
  ///        after an erase.  If there is no more levels, will reinitialize
  ///        the level.
  /// @param the price to search beyond
  /// @param level where to store the results.
  void populate_bid_depth_level_after(const Price& price, DepthLevel& level);

  /// @brief populate an ask depth level after a given price, to rebuild a level
  ///        after an erase.  If there is no more levels, will reinitialize
  ///        the level.
  /// @param the price to search beyond
  /// @param level where to store the results.
  void populate_ask_depth_level_after(const Price& price, DepthLevel& level);

protected:
  /// @brief match a new ask to current bids
  /// @param inbound_order the inbound order
  /// @param inbound_price price of the inbound order
  /// @param bids current bids
  /// @return true if a match occurred 
  virtual bool match_order(Tracker& inbound_order, 
                           const Price& inbound_price, 
                           Bids& bids);

  /// @brief match a new bid to current asks
  /// @param inbound_order the inbound order
  /// @param inbound_price price of the inbound order
  /// @param asks current asks
  /// @return true if a match occurred 
  virtual bool match_order(Tracker& inbound_order, 
                           const Price& inbound_price, 
                           Asks& asks);

  /// @brief perform fill on two orders
  /// @param inbound_tracker the new (or changed) order tracker
  /// @param current_tracker the current order tracker
  void cross_orders(Tracker& inbound_tracker, 
                    Tracker& current_tracker);

  /// @brief perform validation on the order, and create reject callbacks if not
  /// @param order the order to validate
  /// @return true if the order is valid
  virtual bool is_valid(const OrderPtr& order);

  /// @brief perform validation on the order replace, and create reject 
  ///   callbacks if not
  /// @param order the order to validate
  /// @param size_delta the change in size (+ or -)
  /// @param new_price the new order price
  /// @return true if the order replace is valid
  virtual bool is_valid_replace(const OrderPtr& order,
                                int32_t size_delta,
                                Price new_price);

  /// @brief norify child classes one or more callbacks have been added
  virtual void callbacks_added();

  /// @brief find a bid
  void find_bid(const OrderPtr& order, typename Bids::iterator& result);

  /// @brief find an ask
  void find_ask(const OrderPtr& order, typename Asks::iterator& result);
private:
  Bids bids_;
  Asks asks_;
  Callbacks callbacks_;
  OrderBookListener* book_listener_;
  TypedOrderListener* order_listener_;

  Price sort_price(const OrderPtr& order);
};

template <class OrderPtr>
inline
OrderTracker<OrderPtr>::OrderTracker(const OrderPtr& order)
: order_(order),
  filled_qty_(0)
{
}

template <class OrderPtr>
inline void
OrderTracker<OrderPtr>::fill(Quantity qty) 
{
  filled_qty_ += qty;
}

template <class OrderPtr>
inline bool
OrderTracker<OrderPtr>::filled() const
{
  return filled_qty_ >= order_->order_qty();
}

template <class OrderPtr>
inline Quantity
OrderTracker<OrderPtr>::open_qty() const
{
  Quantity original_qty = order_->order_qty();
  // If the order is filled
  if (filled_qty_ >= original_qty) {
    return 0;
  } else {
    return original_qty - filled_qty_;
  }
}

template <class OrderPtr>
inline const OrderPtr&
OrderTracker<OrderPtr>::ptr() const
{
  return order_;
}

template <class OrderPtr>
inline OrderPtr&
OrderTracker<OrderPtr>::ptr()
{
  return order_;
}

template <class OrderPtr>
OrderBook<OrderPtr>::OrderBook()
: book_listener_(NULL),
  order_listener_(NULL)
{
}

template <class OrderPtr>
inline bool
OrderBook<OrderPtr>::add(const OrderPtr& order)
{
  bool matched = false;

  // If the order is invalid, exit
  if (!is_valid(order)) {
    // reject created by is_valid
  } else {
    callbacks_.push_back(TypedCallback::accept(order));
    Price order_price = sort_price(order);

    Tracker inbound(order);
    // Try to match with current orders
    if (order->is_buy()) {
      matched = match_order(inbound, order_price, asks_);
    } else {
      matched = match_order(inbound, order_price, bids_);
    }

    // If order has remaining open quantity
    if (inbound.open_qty()) {
      // If this is a buy order
      if (order->is_buy()) {
        // Insert into bids
        bids_.insert(std::make_pair(order_price, inbound));
      // Else this is a sell order
      } else {
        // Insert into asks
        asks_.insert(std::make_pair(order_price, inbound));
      }
    }
  }

  callbacks_added();
  return matched;
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::cancel(const OrderPtr& order)
{
  bool found = false;
  // If the cancel is a buy order
  if (order->is_buy()) {
    typename Bids::iterator bid;
    find_bid(order, bid);
    if (bid != bids_.end()) {
      // Remove from container for cancel
      bids_.erase(bid);
      found = true;
    }
  // Else the cancel is a sell order
  } else {
    typename Asks::iterator ask;
    find_ask(order, ask);
    if (ask != asks_.end()) {
      // Remove from container for cancel
      asks_.erase(ask);
      found = true;
    }
  } 
  // If the cancel was found, issue callback
  if (found) {
    callbacks_.push_back(TypedCallback::cancel(order));
  } else {
    callbacks_.push_back(TypedCallback::cancel_reject(order, "not found"));
  }
  callbacks_added();
}

template <class OrderPtr>
inline bool
OrderBook<OrderPtr>::replace(
  const OrderPtr& order, 
  int32_t size_delta,
  Price new_price)
{
  bool matched = false;

  // method will push reject callbacks
  if (!is_valid_replace(order, size_delta, new_price)) {
    // reject created by is_valid
  } else {
    bool found = false;
    bool size_decrease = size_delta < 0;

    Quantity new_qty = order->order_qty() + size_delta;
    Price price = (new_price == PRICE_UNCHANGED) ? order->price() : new_price;
    // If the order to replace is a buy order
    if (order->is_buy()) {
      typename Bids::iterator bid;
      find_bid(order, bid);
      // If the order was found
      if (bid != bids_.end()) {
        found = true;
        // If this is a reduction beyond open quantity
        if (size_decrease && (bid->second.open_qty() < std::abs(size_delta))) {
          // Reject the replace
          callbacks_.push_back(
              TypedCallback::replace_reject(order, "not enough open qty"));
        // Else this is fine
        } else {
          // Accept the replace
          callbacks_.push_back(TypedCallback::replace(order, new_qty, price));
          // If the size change will close the order
          if ((size_delta < 0) && (order->open_qty() == Quantity(-size_delta))) {
            bids_.erase(bid); // Remove order
            callbacks_.push_back(TypedCallback::cancel(order));
          }
        }
      }
    // Else the order to replace is a sell order
    } else {
      typename Asks::iterator ask;
      find_ask(order, ask);
      // If the order was found
      if (ask != asks_.end()) {
        found = true;
        // If this is a reduction beyond open quantity
        if (size_decrease && (ask->second.open_qty() < std::abs(size_delta))) {
          // Reject the replace
          callbacks_.push_back(
              TypedCallback::replace_reject(order, "not enough open qty"));
        // Else this is fine
        } else {
          // Accept the replace
          callbacks_.push_back(TypedCallback::replace(order, new_qty, price));
          // If the size change will close the order
          if ((size_delta < 0) && (order->open_qty() == Quantity(-size_delta))) {
            asks_.erase(ask); // Remove order
            callbacks_.push_back(TypedCallback::cancel(order));
          }
        }
      }
    } 

    if (!found) {
      callbacks_.push_back(TypedCallback::replace_reject(order, "not found"));
    }
  }

  callbacks_added();
  return matched;
}

template <class OrderPtr>
inline bool
OrderBook<OrderPtr>::match_order(Tracker& inbound, 
                                 const Price& inbound_price, 
                                 Bids& bids)
{
  bool matched = false;
  typename Bids::iterator bid;

  for (bid = bids.begin(); bid != bids.end(); ) {
    // If the current order has an equal or better price 
    // than the inbound one
    if (bid->first >= inbound_price) {
      // Set return value
      matched =  true;

      // Adjust tracking values for cross
      cross_orders(inbound, bid->second);

      // If the existing order was filled, remove it
      if (bid->second.filled()) {
        bids.erase(bid++);
      } else {
        ++bid;
      }

      // if the inbound order is filled, no more matches are possible
      if (inbound.filled()) {
        break;
      }
    } else {
      // Done, no more existing orders have a matchable price
      break;
    }
  }
  return matched;
}

template <class OrderPtr>
inline bool
OrderBook<OrderPtr>::match_order(Tracker& inbound, 
                                 const Price& inbound_price, 
                                 Asks& asks)
{
  bool matched = false;
  typename Asks::iterator ask;

  for (ask = asks.begin(); ask != asks.end(); ) {
    // If the current order has an equal or better price 
    // than the inbound one
    if (ask->first <= inbound_price) {
      // Set return value
      matched =  true;

      // Adjust tracking values for cross
      cross_orders(inbound, ask->second);

      // If the existing order was filled, remove it
      if (ask->second.filled()) {
        asks.erase(ask++);
      } else {
        ++ask;
      }

      // if the inbound order is filled, no more matches are possible
      if (inbound.filled()) {
        break;
      }
    } else {
      // Done, no more existing orders have a matchable price
      break;
    }
  }
  return matched;
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::cross_orders(Tracker& inbound_tracker, 
                                  Tracker& current_tracker)
{
  Quantity fill_qty = std::min(inbound_tracker.open_qty(), 
                               current_tracker.open_qty());
  Price cross_price = current_tracker.ptr()->price();
  // If current order is a market order, cross at inbound price
  if (MARKET_ORDER_PRICE == cross_price) {
    cross_price = inbound_tracker.ptr()->price();
  }
  
  Cost fill_cost = fill_qty * cross_price;
  inbound_tracker.fill(fill_qty);
  current_tracker.fill(fill_qty);
  callbacks_.push_back(TypedCallback::fill(inbound_tracker.ptr(),
                                           fill_qty,
                                           cross_price,
                                           fill_cost));
  callbacks_.push_back(TypedCallback::fill(current_tracker.ptr(),
                                           fill_qty,
                                           cross_price,
                                           fill_cost));
  callbacks_added();
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::perform_callbacks()
{
  typename Callbacks::iterator cb;
  for (cb = callbacks_.begin(); cb != callbacks_.end(); ++cb) {
    perform_callback(*cb);
  }
  callbacks_.erase(callbacks_.begin(), callbacks_.end());
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::perform_callback(TypedCallback& cb)
{
  // If this is an order callback and I know of an order listener
  if (cb.order_ && order_listener_) {
    switch (cb.type_) {
      case TypedCallback::cb_order_fill:
        order_listener_->on_fill(cb.order_, cb.ref_qty_, cb.ref_cost_);
        break;
      case TypedCallback::cb_order_accept:
        order_listener_->on_accept(cb.order_);
        break;
      case TypedCallback::cb_order_reject:
        order_listener_->on_reject(cb.order_, cb.reject_reason_);
        break;
      case TypedCallback::cb_order_cancel:
        order_listener_->on_cancel(cb.order_);
        break;
      case TypedCallback::cb_order_cancel_reject:
        order_listener_->on_cancel_reject(cb.order_, cb.reject_reason_);
        break;
      case TypedCallback::cb_order_replace:
        order_listener_->on_replace(cb.order_, cb.ref_qty_, cb.ref_price_);
        break;
      case TypedCallback::cb_order_replace_reject:
        order_listener_->on_replace_reject(cb.order_, cb.reject_reason_);
        break;
      case TypedCallback::cb_unknown:
      case TypedCallback::cb_book_update:
      case TypedCallback::cb_depth_update:
      case TypedCallback::cb_bbo_update:
        // Error
        std::runtime_error("Unexpected callback type for order");
        break;
    }
  } else if (book_listener_) {
    // TODO
  }
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::log() const
{
  typename Asks::const_reverse_iterator ask;
  typename Bids::const_iterator bid;
  for (ask = asks_.rbegin(); ask != asks_.rend(); ++ask) {
    std::cout << "  Ask " << ask->second.open_qty() << " @ " << ask->first
                          << std::endl;
  }
  for (bid = bids_.begin(); bid != bids_.end(); ++bid) {
    std::cout << "  Bid " << bid->second.open_qty() << " @ " << bid->first
                          << std::endl;
  }
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::populate_bid_depth_level_after(const Price& price, 
                                                    DepthLevel& level)
{
  // Find the price after the price
  typename Bids::iterator bid = bids_.upper_bound(price);
  // If there was a price after
  if (bid != bids_.end()) {
    // Remember the after price
    Price after_price = bid->first;
    level.init(after_price);
    do {
      // Add this order to the result
      level.add_order(bid->second.open_qty());
    } while ((++bid)->first == after_price);
  // Else there is no price after
  } else {
    level.init(0);
  }
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::populate_ask_depth_level_after(const Price& price, 
                                                    DepthLevel& level)
{
  // Find the price after the price
  typename Bids::iterator ask = asks_.upper_bound(price);
  // If there was a price after
  if (ask != asks_.end()) {
    // Remember the after price
    Price after_price = ask->first;
    level.init(after_price);
    do {
      // Add this order to the result
      level.add_order(ask->second.open_qty());
    } while ((++ask)->first == after_price);
  // Else there is no price after
  } else {
    level.init(0);
  }
}

template <class OrderPtr>
inline bool
OrderBook<OrderPtr>::is_valid(const OrderPtr& order)
{
  if (order->order_qty() == 0) {
    callbacks_.push_back(TypedCallback::reject(order, "size must be positive"));
    return false;
  } else {
    return true;
  }
}

template <class OrderPtr>
inline bool
OrderBook<OrderPtr>::is_valid_replace(
  const OrderPtr& /*order*/,
  int32_t /*size_delta*/,
  Price /*new_price*/)
{
  return true;
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::callbacks_added()
{
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::find_bid(
  const OrderPtr& order,
  typename Bids::iterator& result)
{
  // Find the order search price
  Price search_price = sort_price(order);
  for (result = bids_.find(search_price); result != bids_.end(); ++result) {
    // If this is the correct bid
    if (result->second.ptr() == order) {
      break;
    // Else if this bid's price is too low to match the search price
    } else if (result->first < search_price) {
      break; // No more possible
    }
  }
}

template <class OrderPtr>
inline void
OrderBook<OrderPtr>::find_ask(
  const OrderPtr& order,
  typename Asks::iterator& result)
{
  // Find the order search price
  Price search_price = sort_price(order);
  for (result = asks_.find(search_price); result != asks_.end(); ++result) {
    // If this is the correct ask
    if (result->second.ptr() == order) {
      break;
    // Else if this ask's price is too high to match the search price
    } else if (result->first > search_price) {
      break; // No more possible
    }
  }
} 

template <class OrderPtr>
inline Price
OrderBook<OrderPtr>::sort_price(const OrderPtr& order)
{
  Price result_price = order->price();
  if (MARKET_ORDER_PRICE == result_price) {
    result_price = (order->is_buy() ? MARKET_ORDER_BID_SORT_PRICE :
                                      MARKET_ORDER_ASK_SORT_PRICE);
  }
  return result_price;
}

} }

#endif
