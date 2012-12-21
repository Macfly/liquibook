#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE liquibook_OrderBook
#include <boost/test/unit_test.hpp>
#include "changed_checker.h"
#include "book/order_book.h"
#include "impl/simple_order.h"
#include "impl/simple_order_book.h"
#include <boost/shared_ptr.hpp>

namespace liquibook {

using book::DepthLevel;
using book::OrderBook;
using book::OrderTracker;
using impl::SimpleOrder;

typedef OrderTracker<SimpleOrder*> SimpleTracker;
typedef impl::SimpleOrderBook<5> SimpleOrderBook;
typedef test::ChangedChecker<5> ChangedChecker;
typedef typename SimpleOrderBook::SimpleDepth SimpleDepth;

template <class OrderBook, class OrderPtr>
bool add_and_verify(OrderBook& order_book,
                    const OrderPtr& order,
                    const bool match_expected,
                    const bool complete_expected = false)
{
  const bool matched = order_book.add(order);
  if (matched == match_expected) {
    order_book.perform_callbacks();
    if (complete_expected) {
      // State should be complete
      return impl::os_complete == order->state();
    } else {
      // State should be accepted
      return impl::os_accepted == order->state();
    }
  } else {
    return false;
  }
}

template <class OrderBook, class OrderPtr>
bool cancel_and_verify(OrderBook& order_book,
                       const OrderPtr& order,
                       impl::OrderState expected_state)
{
  order_book.cancel(order);
  order_book.perform_callbacks();
  return expected_state == order->state();
}

template <class OrderBook, class OrderPtr>
bool replace_and_verify(OrderBook& order_book,
                        const OrderPtr& order,
                        int32_t size_change,
                        Price new_price = PRICE_UNCHANGED,
                        impl::OrderState expected_state = impl::os_accepted)
{
  // Calculate
  Quantity expected_order_qty = order->order_qty() + size_change;
  Quantity expected_open_qty = order->open_qty() + size_change;
  Price expected_price = 
      (new_price == PRICE_UNCHANGED) ? order->price() : new_price;

  // Perform
  order_book.replace(order, size_change, new_price);
  order_book.perform_callbacks();

  // Verify
  bool correct = true;
  if (expected_state != order->state()) {
    correct = false;
    std::cout << "State " << order->state() << std::endl;
  }
  if (expected_order_qty != order->order_qty()) {
    correct = false;
    std::cout << "Order Qty " << order->order_qty() << std::endl;
  }
  if (expected_open_qty != order->open_qty()) {
    correct = false;
    std::cout << "Open Qty " << order->open_qty() << std::endl;
  }
  if (expected_price != order->price()) {
    correct = false;
    std::cout << "Price " << order->price() << std::endl;
  }
  return correct;
}

bool verify_depth(const DepthLevel& level,
                  const Price& price,
                  uint32_t count,
                  const Quantity& qty)
{
  bool matched = true;
  if (level.price() != price) {
    std::cout << "Price " << level.price() << std::endl;
    matched = false;
  }
  if (level.order_count() != count) {
    std::cout << "Count " << level.order_count() << std::endl;
    matched = false;
  }
  if (level.aggregate_qty() != qty) {
    std::cout << "Quantity " << level.aggregate_qty() << std::endl;
    matched = false;
  }
  return matched;
}

template <class OrderPtr>
class FillCheck {
public:
  FillCheck(OrderPtr order, 
            Quantity filled_qty,
            Cost filled_cost)
  : order_(order),
    expected_filled_qty_(order->filled_qty() + filled_qty),
    expected_open_qty_(order->order_qty() - expected_filled_qty_),
    expected_filled_cost_(order->filled_cost() + (filled_cost))
  {
  }

  ~FillCheck() {
    verify_filled();
  }

  private:
  OrderPtr order_;
  Quantity expected_filled_qty_;
  Quantity expected_open_qty_;
  Cost expected_filled_cost_;

  void verify_filled() {
    if (expected_filled_qty_ !=  order_->filled_qty()) {
      std::cout << "filled_qty " << order_->filled_qty() 
                << " expected " << expected_filled_qty_ << std::endl;
      throw std::runtime_error("Unexpected filled quantity");
    }
    if (expected_open_qty_ !=  order_->open_qty()) {
      std::cout << "open_qty " << order_->open_qty() 
                << " expected " << expected_open_qty_ << std::endl;
      throw std::runtime_error("Unexpected open quantity");
    }
    if (expected_filled_cost_ !=  order_->filled_cost()) {
      std::cout << "filled_cost " << order_->filled_cost() 
                << " expected " << expected_filled_cost_ << std::endl;
      throw std::runtime_error("Unexpected filled cost");
    }
    if (order_->state() != impl::os_complete && !expected_open_qty_) {
      std::cout << "state " << order_->state() 
                << " expected " << impl::os_complete << std::endl;
      throw std::runtime_error("Unexpected state with no open quantity");
    }
    if (order_->state() != impl::os_accepted && expected_open_qty_) {
      std::cout << "state " << order_->state() 
                << " expected " << impl::os_accepted << std::endl;
      throw std::runtime_error("Unexpected state with open quantity");
    }
  }
};

class DepthCheck {
public:
  DepthCheck(const SimpleDepth& depth) 
  : depth_(depth)
  {
    reset();
  }

  bool verify_bid(const Price& price, int count, const Quantity& qty)
  {
    return verify_depth(*next_bid_++, price, count, qty);
  }

  bool verify_ask(const Price& price, int count, const Quantity& qty)
  {
    return verify_depth(*next_ask_++, price, count, qty);
  }

  void reset()
  {
    next_bid_ = depth_.bids();
    next_ask_ = depth_.asks();
  }

private:
  const SimpleDepth& depth_;
  const DepthLevel* next_bid_;
  const DepthLevel* next_ask_;
};

typedef FillCheck<SimpleOrder*> SimpleFillCheck;

BOOST_AUTO_TEST_CASE(TestBidsMultimapSortCorrect)
{
  SimpleOrderBook::Bids bids;
  SimpleOrder order0(true, 1250, 100);
  SimpleOrder order1(true, 1255, 100);
  SimpleOrder order2(true, 1240, 100);
  SimpleOrder order3(true,    0, 100);
  SimpleOrder order4(true, 1245, 100);

  // Insert out of price order
  bids.insert(std::make_pair(order0.price(), SimpleTracker(&order0)));
  bids.insert(std::make_pair(order1.price(), SimpleTracker(&order1)));
  bids.insert(std::make_pair(order2.price(), SimpleTracker(&order2)));
  bids.insert(std::make_pair(MARKET_ORDER_BID_SORT_PRICE, 
                             SimpleTracker(&order3)));
  bids.insert(std::make_pair(order4.price(), SimpleTracker(&order4)));
  
  // Should access in price order
  SimpleOrder* expected_order[] = {
    &order3, &order1, &order0, &order4, &order2
  };

  SimpleOrderBook::Bids::iterator bid;
  int index = 0;

  for (bid = bids.begin(); bid != bids.end(); ++bid, ++index) {
    if (expected_order[index]->price() == liquibook::MARKET_ORDER_PRICE) {
      BOOST_REQUIRE_EQUAL(MARKET_ORDER_BID_SORT_PRICE, bid->first);
    } else {
      BOOST_REQUIRE_EQUAL(expected_order[index]->price(), bid->first);
    }
    BOOST_REQUIRE_EQUAL(expected_order[index], bid->second.ptr());
  }

  // Should be able to search and find
  BOOST_REQUIRE((bids.upper_bound(1245))->second.ptr()->price() == 1240);
  BOOST_REQUIRE((bids.lower_bound(1245))->second.ptr()->price() == 1245);
}

BOOST_AUTO_TEST_CASE(TestAsksMultimapSortCorrect)
{
  SimpleOrderBook::Asks asks;
  SimpleOrder order0(false, 3250, 100);
  SimpleOrder order1(false, 3235, 800);
  SimpleOrder order2(false, 3230, 200);
  SimpleOrder order3(false,    0, 200);
  SimpleOrder order4(false, 3245, 100);
  SimpleOrder order5(false, 3265, 200);

  // Insert out of price order
  asks.insert(std::make_pair(order0.price(), SimpleTracker(&order0)));
  asks.insert(std::make_pair(order1.price(), SimpleTracker(&order1)));
  asks.insert(std::make_pair(order2.price(), SimpleTracker(&order2)));
  asks.insert(std::make_pair(MARKET_ORDER_ASK_SORT_PRICE, 
                             SimpleTracker(&order3)));
  asks.insert(std::make_pair(order4.price(), SimpleTracker(&order4)));
  asks.insert(std::make_pair(order5.price(), SimpleTracker(&order5)));
  
  // Should access in price order
  SimpleOrder* expected_order[] = {
    &order3, &order2, &order1, &order4, &order0, &order5
  };

  SimpleOrderBook::Asks::iterator ask;
  int index = 0;

  for (ask = asks.begin(); ask != asks.end(); ++ask, ++index) {
    if (expected_order[index]->price() == MARKET_ORDER_PRICE) {
      BOOST_REQUIRE_EQUAL(MARKET_ORDER_ASK_SORT_PRICE, ask->first);
    } else {
      BOOST_REQUIRE_EQUAL(expected_order[index]->price(), ask->first);
    }
    BOOST_REQUIRE_EQUAL(expected_order[index], ask->second.ptr());
  }

  BOOST_REQUIRE((asks.upper_bound(3235))->second.ptr()->price() == 3245);
  BOOST_REQUIRE((asks.lower_bound(3235))->second.ptr()->price() == 3235);
}

BOOST_AUTO_TEST_CASE(TestAddCompleteBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 125100);
    SimpleFillCheck fc2(&ask0, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAddCompleteAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder ask1(false, 1250, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 100, 125000);
    SimpleFillCheck fc2(&bid0, 100, 125000);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAddMultiMatchBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 300);
  SimpleOrder ask2(false, 1251, 200);
  SimpleOrder bid1(true,  1251, 500);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 2, 500));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 500, 1251 * 500);
    SimpleFillCheck fc2(&ask2, 200, 1251 * 200);
    SimpleFillCheck fc3(&ask0, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify remaining
  BOOST_REQUIRE_EQUAL(&ask1, order_book.asks().begin()->second.ptr());
}

BOOST_AUTO_TEST_CASE(TestAddMultiMatchAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 9252, 100);
  SimpleOrder ask0(false, 9251, 300);
  SimpleOrder ask2(false, 9251, 200);
  SimpleOrder ask3(false, 9250, 600);
  SimpleOrder bid0(true,  9250, 100);
  SimpleOrder bid1(true,  9250, 500);
  SimpleOrder bid2(true,  9248, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(9250, 2, 600));
  BOOST_REQUIRE(dc.verify_bid(9248, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(9251, 2, 500));
  BOOST_REQUIRE(dc.verify_ask(9252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask3, 600, 9250 * 600);
    SimpleFillCheck fc2(&bid0, 100, 9250 * 100);
    SimpleFillCheck fc3(&bid1, 500, 9250 * 500);
    BOOST_REQUIRE(add_and_verify(order_book, &ask3, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(9248, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(9251, 2, 500));
  BOOST_REQUIRE(dc.verify_ask(9252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify remaining
  BOOST_REQUIRE_EQUAL(&bid2, order_book.bids().begin()->second.ptr());
}

BOOST_AUTO_TEST_CASE(TestAddPartialMatchBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 7253, 300);
  SimpleOrder ask1(false, 7252, 100);
  SimpleOrder ask2(false, 7251, 200);
  SimpleOrder bid1(true,  7251, 350);
  SimpleOrder bid0(true,  7250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(7250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(7251, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(7252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(7253, 1, 300));

  // Match - partial
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 200, 7251 * 200);
    SimpleFillCheck fc2(&ask2, 200, 7251 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, false));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(7251, 1, 150));
  BOOST_REQUIRE(dc.verify_bid(7250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(7252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(7253, 1, 300));

  // Verify remaining
  BOOST_REQUIRE_EQUAL(&ask1, order_book.asks().begin()->second.ptr());
  BOOST_REQUIRE_EQUAL(&bid1, order_book.bids().begin()->second.ptr());
}

BOOST_AUTO_TEST_CASE(TestAddPartialMatchAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1253, 300);
  SimpleOrder ask1(false, 1251, 400);
  SimpleOrder bid1(true,  1251, 350);
  SimpleOrder bid0(true,  1250, 100);
  SimpleOrder bid2(true,  1250, 200);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 350));
  BOOST_REQUIRE(dc.verify_bid(1250, 2, 300));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Match - partial
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 350, 1251 * 350);
    SimpleFillCheck fc2(&bid1, 350, 1251 * 350);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1,  true, false));
  ); }


  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 2, 300));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  50));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Verify remaining
  BOOST_REQUIRE_EQUAL(&bid0, order_book.bids().begin()->second.ptr());
  BOOST_REQUIRE_EQUAL(&ask1, order_book.asks().begin()->second.ptr());
}

BOOST_AUTO_TEST_CASE(TestAddMultiPartialMatchBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask2(false, 1251, 200);
  SimpleOrder ask0(false, 1251, 300);
  SimpleOrder bid1(true,  1251, 750);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 2, 500));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - partial
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 500, 1251 * 500);
    SimpleFillCheck fc2(&ask0, 300, 1251 * 300);
    SimpleFillCheck fc3(&ask2, 200, 1251 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, false));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 250));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify remaining
  BOOST_REQUIRE_EQUAL(&ask1, order_book.asks().begin()->second.ptr());
  BOOST_REQUIRE_EQUAL(&bid1, order_book.bids().begin()->second.ptr());
}

BOOST_AUTO_TEST_CASE(TestAddMultiPartialMatchAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1253, 300);
  SimpleOrder ask1(false, 1251, 700);
  SimpleOrder bid1(true,  1251, 370);
  SimpleOrder bid2(true,  1251, 200);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 570));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Match - partial
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 570, 1251 * 570);
    SimpleFillCheck fc2(&bid1, 370, 1251 * 370);
    SimpleFillCheck fc3(&bid2, 200, 1251 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1,  true, false));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 130));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Verify remaining
  BOOST_REQUIRE_EQUAL(&bid0, order_book.bids().begin()->second.ptr());
  BOOST_REQUIRE_EQUAL(100, order_book.bids().begin()->second.open_qty());
  BOOST_REQUIRE_EQUAL(&ask1, order_book.asks().begin()->second.ptr());
  BOOST_REQUIRE_EQUAL(130, order_book.asks().begin()->second.open_qty());
}

BOOST_AUTO_TEST_CASE(TestRepeatMatchBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask3(false, 1251, 400);
  SimpleOrder ask2(false, 1251, 200);
  SimpleOrder ask1(false, 1251, 300);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid1(true,  1251, 900);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 900));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));

  // Match - repeated
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 125100);
    SimpleFillCheck fc2(&ask0, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &ask0, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 800));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));

  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 300, 1251 * 300);
    SimpleFillCheck fc2(&ask1, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 500));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));

  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 200, 1251 * 200);
    SimpleFillCheck fc2(&ask2, 200, 1251 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &ask2, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 300));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));

  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 300, 1251 * 300);
    SimpleFillCheck fc2(&ask3, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &ask3, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestRepeatMatchAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false,  1252, 100);
  SimpleOrder ask1(false,  1251, 900);
  SimpleOrder bid0(true, 1251, 100);
  SimpleOrder bid1(true, 1251, 300);
  SimpleOrder bid2(true, 1251, 200);
  SimpleOrder bid3(true, 1251, 400);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 900));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  BOOST_REQUIRE_EQUAL(&ask1, order_book.asks().begin()->second.ptr());

  // Match - repeated
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 100, 125100);
    SimpleFillCheck fc2(&bid0, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &bid0, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 800));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 300, 1251 * 300);
    SimpleFillCheck fc2(&bid1, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 500));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 200, 1251 * 200);
    SimpleFillCheck fc2(&bid2, 200, 1251 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &bid2, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 300));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 300, 1251 * 300);
    SimpleFillCheck fc2(&bid3, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &bid3, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAddMarketOrderBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid1(true,     0, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 125100);
    SimpleFillCheck fc2(&ask0, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
}

BOOST_AUTO_TEST_CASE(TestAddMarketOrderAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false,    0, 100);
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 125100);
    SimpleFillCheck fc2(&ask1, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
}

BOOST_AUTO_TEST_CASE(TestAddMarketOrderBidMultipleMatch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 12520, 300);
  SimpleOrder ask0(false, 12510, 200);
  SimpleOrder bid1(true,      0, 500);
  SimpleOrder bid0(true,  12500, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(12500, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(12510, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(12520, 1, 300));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 500, 12510 * 200 + 12520 * 300);
    SimpleFillCheck fc2(&ask0, 200, 12510 * 200);
    SimpleFillCheck fc3(&ask1, 300, 12520 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(0, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(12500, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(    0, 0,   0));
}

BOOST_AUTO_TEST_CASE(TestAddMarketOrderAskMultipleMatch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 12520, 100);
  SimpleOrder ask1(false,     0, 600);
  SimpleOrder bid1(true,  12510, 200);
  SimpleOrder bid0(true,  12500, 400);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(12510, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(12500, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(12520, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid0, 400, 12500 * 400);
    SimpleFillCheck fc2(&bid1, 200, 12510 * 200);
    SimpleFillCheck fc3(&ask1, 600, 12500 * 400 + 12510 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(    0, 0,   0));
  BOOST_REQUIRE(dc.verify_ask(12520, 1, 100));
}

BOOST_AUTO_TEST_CASE(TestMatchMarketOrderBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1253, 100);
  SimpleOrder bid1(true,     0, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(0, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 125300);
    SimpleFillCheck fc2(&ask0, 100, 125300);
    BOOST_REQUIRE(add_and_verify(order_book, &ask0, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(0, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
}

BOOST_AUTO_TEST_CASE(TestMatchMarketOrderAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false,    0, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid0, 100, 125000);
    SimpleFillCheck fc2(&ask1, 100, 125000);
    BOOST_REQUIRE(add_and_verify(order_book, &bid0, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
}

BOOST_AUTO_TEST_CASE(TestMatchMultipleMarketOrderBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1253, 400);
  SimpleOrder bid1(true,     0, 100);
  SimpleOrder bid2(true,     0, 200);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(0, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 1253 * 100);
    SimpleFillCheck fc2(&bid2, 200, 1253 * 200);
    SimpleFillCheck fc3(&ask0, 300, 1253 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &ask0, true, false));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
}


BOOST_AUTO_TEST_CASE(TestMatchMultipleMarketOrderAsk)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask2(false,    0, 400);
  SimpleOrder ask1(false,    0, 100);
  SimpleOrder bid0(true,  1250, 300);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));

  // Match - partiaL
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid0, 300, 1250 * 300);
    SimpleFillCheck fc2(&ask1, 100, 1250 * 100);
    SimpleFillCheck fc3(&ask2, 200, 1250 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &bid0, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
}

BOOST_AUTO_TEST_CASE(TestCancelBid)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Cancel bid
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid0, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(0,    0,   0));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestCancelAskAndMatch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid2(true,  1252, 100);
  SimpleOrder bid0(true,  1250, 100);
  SimpleOrder bid1(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 2, 200));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Cancel bid
  BOOST_REQUIRE(cancel_and_verify(order_book, &ask0, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 2, 200));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));

  // Match - partiaL
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid2, 100, 1252 * 100);
    SimpleFillCheck fc2(&ask1, 100, 1252 * 100);
    BOOST_REQUIRE(add_and_verify(order_book, &bid2, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 2, 200));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));

  // Cancel bid
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid0, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(0, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestCancelBidFail)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder ask1(false, 1250, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1, 100, 125000);
    SimpleFillCheck fc2(&bid0, 100, 125000);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));

  // Cancel a filled order
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid0, impl::os_complete));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
}

BOOST_AUTO_TEST_CASE(TestCancelAskFail)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 125100);
    SimpleFillCheck fc2(&ask0, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));

  // Cancel a filled order
  BOOST_REQUIRE(cancel_and_verify(order_book, &ask0, impl::os_complete));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
}

BOOST_AUTO_TEST_CASE(TestCancelBidRestore)
{
  SimpleOrderBook order_book;
  SimpleOrder ask10(false, 1258, 600);
  SimpleOrder ask9(false,  1257, 700);
  SimpleOrder ask8(false,  1256, 100);
  SimpleOrder ask7(false,  1256, 100);
  SimpleOrder ask6(false,  1255, 500);
  SimpleOrder ask5(false,  1255, 200);
  SimpleOrder ask4(false,  1254, 300);
  SimpleOrder ask3(false,  1252, 200);
  SimpleOrder ask2(false,  1252, 100);
  SimpleOrder ask1(false,  1251, 400);
  SimpleOrder ask0(false,  1250, 500);

  SimpleOrder bid0(true,   1249, 100);
  SimpleOrder bid1(true,   1249, 200);
  SimpleOrder bid2(true,   1249, 200);
  SimpleOrder bid3(true,   1248, 400);
  SimpleOrder bid4(true,   1246, 600);
  SimpleOrder bid5(true,   1246, 500);
  SimpleOrder bid6(true,   1245, 200);
  SimpleOrder bid7(true,   1245, 100);
  SimpleOrder bid8(true,   1245, 200);
  SimpleOrder bid9(true,   1244, 700);
  SimpleOrder bid10(true,  1244, 300);
  SimpleOrder bid11(true,  1242, 300);
  SimpleOrder bid12(true,  1241, 400);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid11, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid12, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(13, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(11, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));

  // Cancel a bid level (erase)
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid3, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_bid(1242, 1,  300)); // Restored
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));
  
  // Cancel common bid levels (not erased)
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid7, impl::os_cancelled));
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid4, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1246, 1,  500)); // Cxl 600
  BOOST_REQUIRE(dc.verify_bid(1245, 2,  400)); // Cxl 100
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_bid(1242, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));

  // Cancel the best bid level (erased)
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid1, impl::os_cancelled));
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid0, impl::os_cancelled));
  BOOST_REQUIRE(cancel_and_verify(order_book, &bid2, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1246, 1,  500));
  BOOST_REQUIRE(dc.verify_bid(1245, 2,  400));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_bid(1242, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1241, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));
}

BOOST_AUTO_TEST_CASE(TestCancelAskRestore)
{
  SimpleOrderBook order_book;
  SimpleOrder ask10(false, 1258, 600);
  SimpleOrder ask9(false,  1257, 700);
  SimpleOrder ask8(false,  1256, 100);
  SimpleOrder ask7(false,  1256, 100);
  SimpleOrder ask6(false,  1255, 500);
  SimpleOrder ask5(false,  1255, 200);
  SimpleOrder ask4(false,  1254, 300);
  SimpleOrder ask3(false,  1252, 200);
  SimpleOrder ask2(false,  1252, 100);
  SimpleOrder ask1(false,  1251, 400);
  SimpleOrder ask0(false,  1250, 500);

  SimpleOrder bid0(true,   1249, 100);
  SimpleOrder bid1(true,   1249, 200);
  SimpleOrder bid2(true,   1249, 200);
  SimpleOrder bid3(true,   1248, 400);
  SimpleOrder bid4(true,   1246, 600);
  SimpleOrder bid5(true,   1246, 500);
  SimpleOrder bid6(true,   1245, 200);
  SimpleOrder bid7(true,   1245, 100);
  SimpleOrder bid8(true,   1245, 200);
  SimpleOrder bid9(true,   1244, 700);
  SimpleOrder bid10(true,  1244, 300);
  SimpleOrder bid11(true,  1242, 300);
  SimpleOrder bid12(true,  1241, 400);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid11, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid12, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(13, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(11, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));

  // Cancel an ask level (erase)
  BOOST_REQUIRE(cancel_and_verify(order_book, &ask1, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200)); // Restored

  // Cancel common ask levels (not erased)
  BOOST_REQUIRE(cancel_and_verify(order_book, &ask2, impl::os_cancelled));
  BOOST_REQUIRE(cancel_and_verify(order_book, &ask6, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1252, 1,  200)); // Cxl 100
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 1,  200)); // Cxl 500
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200));

  // Cancel the best ask level (erased)
  BOOST_REQUIRE(cancel_and_verify(order_book, &ask0, impl::os_cancelled));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_ask(1252, 1,  200));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 1,  200));
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200));
  BOOST_REQUIRE(dc.verify_ask(1257, 1,  700)); // Restored
}

BOOST_AUTO_TEST_CASE(TestFillCompleteBidRestoreDepth)
{
  SimpleOrderBook order_book;
  SimpleOrder ask10(false, 1258, 600);
  SimpleOrder ask9(false,  1257, 700);
  SimpleOrder ask8(false,  1256, 100);
  SimpleOrder ask7(false,  1256, 100);
  SimpleOrder ask6(false,  1255, 500);
  SimpleOrder ask5(false,  1255, 200);
  SimpleOrder ask4(false,  1254, 300);
  SimpleOrder ask3(false,  1252, 200);
  SimpleOrder ask2(false,  1252, 100);
  SimpleOrder ask1(false,  1251, 400);
  SimpleOrder ask0(false,  1250, 500);

  SimpleOrder bid0(true,   1249, 100);
  SimpleOrder bid1(true,   1249, 200);
  SimpleOrder bid2(true,   1249, 200);
  SimpleOrder bid3(true,   1248, 400);
  SimpleOrder bid4(true,   1246, 600);
  SimpleOrder bid5(true,   1246, 500);
  SimpleOrder bid6(true,   1245, 200);
  SimpleOrder bid7(true,   1245, 100);
  SimpleOrder bid8(true,   1245, 200);
  SimpleOrder bid9(true,   1244, 700);
  SimpleOrder bid10(true,  1244, 300);
  SimpleOrder bid11(true,  1242, 300);
  SimpleOrder bid12(true,  1241, 400);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid11, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid12, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(13, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(11, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));

  // Fill the top bid level (erase) and add an ask level (insert)
  SimpleOrder cross_ask(false,  1249, 800);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid0,      100, 1249 * 100);
    SimpleFillCheck fc2(&bid1,      200, 1249 * 200);
    SimpleFillCheck fc3(&bid2,      200, 1249 * 200);
    SimpleFillCheck fc4(&cross_ask, 500, 1249 * 500);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_ask, true, false));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_bid(1242, 1,  300)); // Restored
  BOOST_REQUIRE(dc.verify_ask(1249, 1,  300)); // Inserted
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  
  // Fill the top bid level (erase) but do not add an ask level (no insert)
  SimpleOrder cross_ask2(false,  1248, 400);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid3,       400, 1248 * 400);
    SimpleFillCheck fc4(&cross_ask2, 400, 1248 * 400);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_ask2, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_bid(1242, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1241, 1,  400)); // Restored
  BOOST_REQUIRE(dc.verify_ask(1249, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));

  // Fill the top bid level (erase) and add ask level (insert),
  //    but nothing to restore
  SimpleOrder cross_ask3(false,  1246, 2400);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid4,        600, 1246 * 600);
    SimpleFillCheck fc2(&bid5,        500, 1246 * 500);
    SimpleFillCheck fc3(&cross_ask3, 1100, 1246 * 1100);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_ask3, true, false));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_bid(1242, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1241, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,    0)); // Nothing to restore
  BOOST_REQUIRE(dc.verify_ask(1246, 1, 1300));
  BOOST_REQUIRE(dc.verify_ask(1249, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));

  // Partial fill the top bid level (reduce) 
  SimpleOrder cross_ask4(false,  1245, 250);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid6,        200, 1245 * 200);
    SimpleFillCheck fc2(&bid7,         50, 1245 *  50);
    SimpleFillCheck fc3(&cross_ask4,  250, 1245 * 250);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_ask4, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1245, 2,  250)); // 1 filled, 1 reduced
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_bid(1242, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1241, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,    0));
  BOOST_REQUIRE(dc.verify_ask(1246, 1, 1300));
  BOOST_REQUIRE(dc.verify_ask(1249, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
}

BOOST_AUTO_TEST_CASE(TestFillCompleteAskRestoreDepth)
{
  SimpleOrderBook order_book;
  SimpleOrder ask10(false, 1258, 600);
  SimpleOrder ask9(false,  1257, 700);
  SimpleOrder ask8(false,  1256, 100);
  SimpleOrder ask7(false,  1256, 100);
  SimpleOrder ask6(false,  1255, 500);
  SimpleOrder ask5(false,  1255, 200);
  SimpleOrder ask4(false,  1254, 300);
  SimpleOrder ask3(false,  1252, 200);
  SimpleOrder ask2(false,  1252, 100);
  SimpleOrder ask1(false,  1251, 400);
  SimpleOrder ask0(false,  1250, 500);

  SimpleOrder bid0(true,   1249, 100);
  SimpleOrder bid1(true,   1249, 200);
  SimpleOrder bid2(true,   1249, 200);
  SimpleOrder bid3(true,   1248, 400);
  SimpleOrder bid4(true,   1246, 600);
  SimpleOrder bid5(true,   1246, 500);
  SimpleOrder bid6(true,   1245, 200);
  SimpleOrder bid7(true,   1245, 100);
  SimpleOrder bid8(true,   1245, 200);
  SimpleOrder bid9(true,   1244, 700);
  SimpleOrder bid10(true,  1244, 300);
  SimpleOrder bid11(true,  1242, 300);
  SimpleOrder bid12(true,  1241, 400);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid4,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid5,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid6,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid7,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid8,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid9,  false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid10, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid11, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid12, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(13, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(11, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1244, 2, 1000));
  BOOST_REQUIRE(dc.verify_ask(1250, 1,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));

  // Fill the top ask level (erase) and add a bid level (insert)
  SimpleOrder cross_bid(true,  1250, 800);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask0,      500, 1250 * 500);
    SimpleFillCheck fc4(&cross_bid, 500, 1250 * 500);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_bid, true, false));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_ask(1251, 1,  400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200)); // Restored

  // Fill the top ask level (erase) but do not add an bid level (no insert)
  SimpleOrder cross_bid2(true,  1251, 400);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask1,       400, 1251 * 400);
    SimpleFillCheck fc4(&cross_bid2, 400, 1251 * 400);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_bid2, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_bid(1245, 3,  500));
  BOOST_REQUIRE(dc.verify_ask(1252, 2,  300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200));
  BOOST_REQUIRE(dc.verify_ask(1257, 1,  700)); // Restored

  // Fill the top ask level (erase) and add bid level (insert),
  //    but nothing to restore
  SimpleOrder cross_bid3(true,  1252, 2400);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask2,        100, 1252 * 100);
    SimpleFillCheck fc2(&ask3,        200, 1252 * 200);
    SimpleFillCheck fc3(&cross_bid3,  300, 1252 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_bid3, true, false));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1252, 1, 2100)); // Insert
  BOOST_REQUIRE(dc.verify_bid(1250, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_ask(1254, 1,  300));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200));
  BOOST_REQUIRE(dc.verify_ask(1257, 1,  700));
  BOOST_REQUIRE(dc.verify_ask(1258, 1,  600)); // Restored

  // Fill the top ask level (erase) but nothing to restore
  SimpleOrder cross_bid4(true,  1254, 300);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc2(&ask4,        300, 1254 * 300);
    SimpleFillCheck fc3(&cross_bid4,  300, 1254 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_bid4, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1252, 1, 2100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_ask(1255, 2,  700));
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200));
  BOOST_REQUIRE(dc.verify_ask(1257, 1,  700));
  BOOST_REQUIRE(dc.verify_ask(1258, 1,  600));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,    0)); // Nothing to restore

  // Partial fill the top ask level (reduce) 
  SimpleOrder cross_bid5(true,  1255, 550);
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask5,        200, 1255 * 200);
    SimpleFillCheck fc2(&ask6,        350, 1255 * 350);
    SimpleFillCheck fc3(&cross_bid5,  550, 1255 * 550);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_bid5, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1252, 1, 2100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1,  300));
  BOOST_REQUIRE(dc.verify_bid(1249, 3,  500));
  BOOST_REQUIRE(dc.verify_bid(1248, 1,  400));
  BOOST_REQUIRE(dc.verify_bid(1246, 2, 1100));
  BOOST_REQUIRE(dc.verify_ask(1255, 1,  150)); // 1 filled, 1 reduced
  BOOST_REQUIRE(dc.verify_ask(1256, 2,  200));
  BOOST_REQUIRE(dc.verify_ask(1257, 1,  700));
  BOOST_REQUIRE(dc.verify_ask(1258, 1,  600));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,    0));
}

typedef boost::shared_ptr<SimpleOrder> SimpleOrderPtr;
class SharedPtrOrderBook : public OrderBook<SimpleOrderPtr>
{
  virtual void perform_callback(OrderBook<SimpleOrderPtr>::TypedCallback& cb)
  {
    switch(cb.type_) {
      case TypedCallback::cb_order_accept:
        cb.order_->accept();
        break;
      case TypedCallback::cb_order_fill:
        cb.order_->fill(cb.ref_qty_, cb.ref_cost_, 0);
        break;
      case TypedCallback::cb_order_cancel:
        cb.order_->cancel();
        break;
      case TypedCallback::cb_order_replace:
        cb.order_->replace(cb.ref_qty_, cb.ref_price_);
        break;
      default:
        // Nothing
        break;
    }
  }
};

typedef FillCheck<SimpleOrderPtr> SharedFillCheck;

BOOST_AUTO_TEST_CASE(TestSharedPointerBuild)
{
  SharedPtrOrderBook order_book;
  SimpleOrderPtr ask1(new SimpleOrder(false, 1252, 100));
  SimpleOrderPtr ask0(new SimpleOrder(false, 1251, 100));
  SimpleOrderPtr bid1(new SimpleOrder(true,  1251, 100));
  SimpleOrderPtr bid0(new SimpleOrder(true,  1250, 100));

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SharedFillCheck fc1(bid1, 100, 125100);
    SharedFillCheck fc2(ask0, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, bid1, true, true));
  ); }

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestSharedCancelBid)
{
  SharedPtrOrderBook order_book;
  SimpleOrderPtr ask1(new SimpleOrder(false, 1252, 100));
  SimpleOrderPtr ask0(new SimpleOrder(false, 1251, 100));
  SimpleOrderPtr bid0(new SimpleOrder(true,  1250, 100));

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Cancel bid
  BOOST_REQUIRE(cancel_and_verify(order_book, bid0, impl::os_cancelled));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(0, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestPopulateLevels)
{
  SimpleOrderBook order_book;
  SimpleOrder ask6(false, 1254, 300);
  SimpleOrder ask5(false, 1254, 300);
  SimpleOrder ask4(false, 1254, 100);
  SimpleOrder ask3(false, 1252, 400);
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask2(false, 1251, 200);
  SimpleOrder ask0(false, 1251, 300);

  SimpleOrder bid6(true,  1251, 500);

  SimpleOrder bid0(true,  1250, 100);
  SimpleOrder bid2(true,  1250, 300);
  SimpleOrder bid4(true,  1248, 100);
  SimpleOrder bid3(true,  1248, 200);
  SimpleOrder bid5(true,  1247, 100);
  SimpleOrder bid1(true,  1246, 200);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid4, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid5, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask4, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask5, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask6, false));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid6, 500, 1251 * 500);
    SimpleFillCheck fc2(&ask2, 200, 1251 * 200);
    SimpleFillCheck fc3(&ask0, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &bid6, true, true));
  ); }

  // Verify depth building
  DepthLevel level;
  order_book.populate_bid_depth_level_after(1251, level);
  BOOST_REQUIRE(verify_depth(level, 1250, 2, 400));
  order_book.populate_bid_depth_level_after(1250, level);
  BOOST_REQUIRE(verify_depth(level, 1248, 2, 300));
  order_book.populate_bid_depth_level_after(1248, level);
  BOOST_REQUIRE(verify_depth(level, 1247, 1, 100));
  order_book.populate_bid_depth_level_after(1247, level);
  BOOST_REQUIRE(verify_depth(level, 1246, 1, 200));
  order_book.populate_ask_depth_level_after(1251, level);
  BOOST_REQUIRE(verify_depth(level, 1252, 2, 500));
  order_book.populate_ask_depth_level_after(1252, level);
  BOOST_REQUIRE(verify_depth(level, 1254, 3, 700));
}

BOOST_AUTO_TEST_CASE(TestReplaceSizeIncrease)
{
  SimpleOrderBook order_book;
  ChangedChecker cc(order_book.depth());
  SimpleOrder ask0(false, 1252, 300);
  SimpleOrder ask1(false, 1251, 200);
  SimpleOrder bid0(true,  1250, 100);
  SimpleOrder bid1(true,  1249, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 300));

  // Verify changed stamps
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 0, 0, 0));

  // Replace size
  BOOST_REQUIRE(replace_and_verify(order_book, &bid0, 25));
  BOOST_REQUIRE(replace_and_verify(order_book, &ask0, 50));

  // Verify changed stamps
  BOOST_REQUIRE(cc.verify_bid_stamps(5, 2, 0, 0, 0));
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 6, 0, 0, 0));

  // Verify orders
  BOOST_REQUIRE_EQUAL(125, bid0.order_qty());
  BOOST_REQUIRE_EQUAL(350, ask0.order_qty());
  
  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 125));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 350));
}

BOOST_AUTO_TEST_CASE(TestReplaceSizeDecrease)
{
  SimpleOrderBook order_book;
  ChangedChecker cc(order_book.depth());
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder ask0(false, 1252, 300);
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 500));

  // Verify changed stamps
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 2, 0, 0, 0));
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 0, 0, 0, 0));

  // Replace size
  BOOST_REQUIRE(replace_and_verify(order_book, &bid0, -60));
  BOOST_REQUIRE(replace_and_verify(order_book, &ask0, -150));

  // Verify orders
  BOOST_REQUIRE_EQUAL(40, bid0.order_qty());
  BOOST_REQUIRE_EQUAL(150, ask0.order_qty());
  
  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1,  40));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 350));

  // Verify changed stamps
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 5, 0, 0, 0));
  BOOST_REQUIRE(cc.verify_ask_stamps(6, 0, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestReplaceSizeDecreaseCancel)
{
  SimpleOrderBook order_book;
  ChangedChecker cc(order_book.depth());
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder ask0(false, 1252, 300);
  SimpleOrder bid1(true,  1251, 400);
  SimpleOrder bid0(true,  1250, 100);
  SimpleOrder bid2(true,  1249, 700);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 500));
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 400));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 700));

  // Verify changed stamps
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 2, 3, 0, 0));
  BOOST_REQUIRE(cc.verify_ask_stamps(5, 0, 0, 0, 0));

  // Partial Fill existing book
  SimpleOrder cross_bid(true,  1252, 125);
  SimpleOrder cross_ask(false, 1251, 100);
  
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&cross_bid, 125, 1252 * 125);
    SimpleFillCheck fc2(&ask0,      125, 1252 * 125);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_bid, true, true));
  ); }
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&cross_ask, 100, 1251 * 100);
    SimpleFillCheck fc2(&bid1,      100, 1251 * 100);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_ask, true, true));
  ); }

  // TODO Verify changed stamps
  // Wait for accept_no_insert change
  //BOOST_REQUIRE(cc.verify_bid_stamps(7, 2, 3, 0, 0));
  //BOOST_REQUIRE(cc.verify_ask_stamps(5, 0, 0, 0, 0));

  // Verify quantity
  BOOST_REQUIRE_EQUAL(175, ask0.open_qty());
  BOOST_REQUIRE_EQUAL(300, bid1.open_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 300));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 375));

  // Replace size - cancel
  BOOST_REQUIRE(replace_and_verify(
      order_book, &ask0, -175, PRICE_UNCHANGED, impl::os_cancelled)); 

  // Verify orders
  BOOST_REQUIRE_EQUAL(125, ask0.order_qty());
  BOOST_REQUIRE_EQUAL(0, ask0.open_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 300));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));

  // Replace size - reduce level
  BOOST_REQUIRE(replace_and_verify(
      order_book, &bid1, -100, PRICE_UNCHANGED, impl::os_accepted)); 

  // Verify orders
  BOOST_REQUIRE_EQUAL(300, bid1.order_qty());
  BOOST_REQUIRE_EQUAL(200, bid1.open_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));

  // Replace size - cancel and erase level
  BOOST_REQUIRE(replace_and_verify(
      order_book, &bid1, -200, PRICE_UNCHANGED, impl::os_cancelled)); 

  // Verify orders
  BOOST_REQUIRE_EQUAL(100, bid1.order_qty());
  BOOST_REQUIRE_EQUAL(0, bid1.open_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
}

BOOST_AUTO_TEST_CASE(TestReplaceSizeDecreaseTooMuch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder ask0(false, 1252, 300);
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 500));

  SimpleOrder cross_bid(true,  1252, 200);
  // Partial fill existing order
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&cross_bid, 200, 1252 * 200);
    SimpleFillCheck fc2(&ask0,      200, 1252 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &cross_bid, true, true));
  ); }

  // Verify open quantity
  BOOST_REQUIRE_EQUAL(100, ask0.open_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 300));

  // Replace size - not enough left
  order_book.replace(&ask0, -150, PRICE_UNCHANGED);
  order_book.perform_callbacks();

  // Verify orders
  BOOST_REQUIRE_EQUAL(100, ask0.open_qty());
  BOOST_REQUIRE_EQUAL(300, ask0.order_qty());

  // Verify open quantity unchanged
  BOOST_REQUIRE_EQUAL(impl::os_accepted, ask0.state());
  BOOST_REQUIRE_EQUAL(100, ask0.open_qty());

  // Verify depth unchanged
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 300));
}

BOOST_AUTO_TEST_CASE(TestReplaceSizeIncreaseDecrease)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder ask0(false, 1251, 300);
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 300));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));

  // Replace size
  BOOST_REQUIRE(replace_and_verify(order_book, &ask0, 50));
  BOOST_REQUIRE(replace_and_verify(order_book, &bid0, 25));

  BOOST_REQUIRE(replace_and_verify(order_book, &ask0, -100));
  BOOST_REQUIRE(replace_and_verify(order_book, &bid0, 25));

  BOOST_REQUIRE(replace_and_verify(order_book, &ask0, 300));
  BOOST_REQUIRE(replace_and_verify(order_book, &bid0, -75));

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 75));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 550));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
}

BOOST_AUTO_TEST_CASE(TestReplaceBidPriceChange)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1253, 300);
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder bid1(true,  1251, 140);
  SimpleOrder bid0(true,  1250, 120);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Replace price increase
  BOOST_REQUIRE(replace_and_verify(order_book, &bid0, SIZE_UNCHANGED, 1251));

  // Verify price change in book
  SimpleOrderBook::Bids::const_iterator bid = order_book.bids().begin();
  BOOST_REQUIRE_EQUAL(1251, bid->first);
  BOOST_REQUIRE_EQUAL(&bid1, bid->second.ptr());
  BOOST_REQUIRE_EQUAL(1251, (++bid)->first);
  BOOST_REQUIRE_EQUAL(&bid0, bid->second.ptr());
  BOOST_REQUIRE(order_book.bids().end() == ++bid);

  // Verify order
  BOOST_REQUIRE_EQUAL(1251, bid0.price());
  BOOST_REQUIRE_EQUAL(120, bid0.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 260));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Replace price decrease
  BOOST_REQUIRE(replace_and_verify(order_book, &bid1, SIZE_UNCHANGED, 1250));

  // Verify price change in book
  bid = order_book.bids().begin();
  BOOST_REQUIRE_EQUAL(1251, bid->first);
  BOOST_REQUIRE_EQUAL(&bid0, bid->second.ptr());
  BOOST_REQUIRE_EQUAL(1250, (++bid)->first);
  BOOST_REQUIRE_EQUAL(&bid1, bid->second.ptr());
  BOOST_REQUIRE(order_book.bids().end() == ++bid);

  // Verify order
  BOOST_REQUIRE_EQUAL(1250, bid1.price());
  BOOST_REQUIRE_EQUAL(140, bid1.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 120));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(   0, 0,   0));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));
}

BOOST_AUTO_TEST_CASE(TestReplaceAskPriceChange)
{
  SimpleOrderBook order_book;
  ChangedChecker cc(order_book.depth());

  SimpleOrder ask0(false, 1253, 300);
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder bid1(true,  1251, 140);
  SimpleOrder bid0(true,  1250, 120);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify changed stamps
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 2, 0, 0, 0));
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 0, 0, 0));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Replace price increase 1252 -> 1253
  BOOST_REQUIRE(replace_and_verify(order_book, &ask1, SIZE_UNCHANGED, 1253));

  // TODO: Verify changed stamps
  // BOOST_REQUIRE(cc.verify_bid_stamps(2, 2, 0, 0, 0));
  // BOOST_REQUIRE(cc.verify_ask_stamps(5, 6, 0, 0, 0));

  // Verify price change in book
  SimpleOrderBook::Asks::const_iterator ask = order_book.asks().begin();
  BOOST_REQUIRE_EQUAL(1253, ask->first);
  BOOST_REQUIRE_EQUAL(&ask0, ask->second.ptr());
  BOOST_REQUIRE_EQUAL(1253, (++ask)->first);
  BOOST_REQUIRE_EQUAL(&ask1, ask->second.ptr());
  BOOST_REQUIRE(order_book.asks().end() == ++ask);

  // Verify order
  BOOST_REQUIRE_EQUAL(1253, ask1.price());
  BOOST_REQUIRE_EQUAL(200, ask1.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_ask(1253, 2, 500));

  // Replace price decrease 1253 -> 1252
  BOOST_REQUIRE(replace_and_verify(order_book, &ask0, SIZE_UNCHANGED, 1252));

  // Verify price change in book
  ask = order_book.asks().begin();
  BOOST_REQUIRE_EQUAL(1252, ask->first);
  BOOST_REQUIRE_EQUAL(&ask0, ask->second.ptr());
  BOOST_REQUIRE_EQUAL(1253, (++ask)->first);
  BOOST_REQUIRE_EQUAL(&ask1, ask->second.ptr());
  BOOST_REQUIRE(order_book.asks().end() == ++ask);

  // Verify order
  BOOST_REQUIRE_EQUAL(1252, ask0.price());
  BOOST_REQUIRE_EQUAL(300, ask0.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 300));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(   0, 0,   0));
}

BOOST_AUTO_TEST_CASE(TestReplaceBidPriceChangeErase)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1253, 300);
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder bid1(true,  1251, 140);
  SimpleOrder bid0(true,  1250, 120);
  SimpleOrder bid2(true,  1249, 100);
  SimpleOrder bid3(true,  1248, 200);
  SimpleOrder bid4(true,  1247, 400);
  SimpleOrder bid5(true,  1246, 800);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid4, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid5, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1248, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1247, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Replace price increase 1250 -> 1251
  BOOST_REQUIRE(replace_and_verify(order_book, &bid0, SIZE_UNCHANGED, 1251));

  // Verify price change in book
  SimpleOrderBook::Bids::const_iterator bid = order_book.bids().begin();
  BOOST_REQUIRE_EQUAL(1251, bid->first);
  BOOST_REQUIRE_EQUAL(&bid1, bid->second.ptr());
  BOOST_REQUIRE_EQUAL(1251, (++bid)->first);
  BOOST_REQUIRE_EQUAL(&bid0, bid->second.ptr());
  BOOST_REQUIRE_EQUAL(1249, (++bid)->first);
  BOOST_REQUIRE_EQUAL(&bid2, bid->second.ptr());

  // Verify order
  BOOST_REQUIRE_EQUAL(1251, bid0.price());
  BOOST_REQUIRE_EQUAL(120, bid0.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 260));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1248, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1247, 1, 400));
  BOOST_REQUIRE(dc.verify_bid(1246, 1, 800));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));

  // Replace price decrease 1251 -> 1250
  BOOST_REQUIRE(replace_and_verify(order_book, &bid1, SIZE_UNCHANGED, 1250));

  // Verify price change in book
  bid = order_book.bids().begin();
  BOOST_REQUIRE_EQUAL(1251, bid->first);
  BOOST_REQUIRE_EQUAL(&bid0, bid->second.ptr());
  BOOST_REQUIRE_EQUAL(1250, (++bid)->first);
  BOOST_REQUIRE_EQUAL(&bid1, bid->second.ptr());
  BOOST_REQUIRE_EQUAL(1249, (++bid)->first);
  BOOST_REQUIRE_EQUAL(&bid2, bid->second.ptr());

  // Verify order
  BOOST_REQUIRE_EQUAL(1250, bid1.price());
  BOOST_REQUIRE_EQUAL(140, bid1.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 120));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1249, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1248, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1247, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));
}

BOOST_AUTO_TEST_CASE(TestReplaceAskPriceChangeErase)
{
  SimpleOrderBook order_book;
  SimpleOrder ask5(false, 1258, 304);
  SimpleOrder ask4(false, 1256, 330);
  SimpleOrder ask3(false, 1255, 302);
  SimpleOrder ask2(false, 1254, 310);
  SimpleOrder ask0(false, 1253, 300);
  SimpleOrder ask1(false, 1252, 200);
  SimpleOrder bid1(true,  1251, 140);
  SimpleOrder bid0(true,  1250, 120);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask4, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask5, false));

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 300));
  BOOST_REQUIRE(dc.verify_ask(1254, 1, 310));
  BOOST_REQUIRE(dc.verify_ask(1255, 1, 302));
  BOOST_REQUIRE(dc.verify_ask(1256, 1, 330));

  // Replace price increase 1252 -> 1253
  BOOST_REQUIRE(replace_and_verify(order_book, &ask1, SIZE_UNCHANGED, 1253));

  // Verify price change in book
  SimpleOrderBook::Asks::const_iterator ask = order_book.asks().begin();
  BOOST_REQUIRE_EQUAL(1253, ask->first);
  BOOST_REQUIRE_EQUAL(&ask0, ask->second.ptr());
  BOOST_REQUIRE_EQUAL(1253, (++ask)->first);
  BOOST_REQUIRE_EQUAL(&ask1, ask->second.ptr());
  BOOST_REQUIRE_EQUAL(1254, (++ask)->first);
  BOOST_REQUIRE_EQUAL(&ask2, ask->second.ptr());

  // Verify order
  BOOST_REQUIRE_EQUAL(1253, ask1.price());
  BOOST_REQUIRE_EQUAL(200, ask1.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_ask(1253, 2, 500));
  BOOST_REQUIRE(dc.verify_ask(1254, 1, 310));
  BOOST_REQUIRE(dc.verify_ask(1255, 1, 302));
  BOOST_REQUIRE(dc.verify_ask(1256, 1, 330));
  BOOST_REQUIRE(dc.verify_ask(1258, 1, 304));

  // Replace price decrease 1253 -> 1252
  BOOST_REQUIRE(replace_and_verify(order_book, &ask0, SIZE_UNCHANGED, 1252));

  // Verify price change in book
  ask = order_book.asks().begin();
  BOOST_REQUIRE_EQUAL(1252, ask->first);
  BOOST_REQUIRE_EQUAL(&ask0, ask->second.ptr());
  BOOST_REQUIRE_EQUAL(1253, (++ask)->first);
  BOOST_REQUIRE_EQUAL(&ask1, ask->second.ptr());
  BOOST_REQUIRE_EQUAL(1254, (++ask)->first);
  BOOST_REQUIRE_EQUAL(&ask2, ask->second.ptr());

  // Verify order
  BOOST_REQUIRE_EQUAL(1252, ask0.price());
  BOOST_REQUIRE_EQUAL(300, ask0.order_qty());

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 140));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 120));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 300));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1254, 1, 310));
  BOOST_REQUIRE(dc.verify_ask(1255, 1, 302));
  BOOST_REQUIRE(dc.verify_ask(1256, 1, 330));
}

// A potential problem
// When restroing a level into the depth, the orders (and thus the restored
// level already reflect the post-fill quantity, but the fill callback has 
// yet to be processed.  As such, a multilevel fill can have fills at the 
// restoration price double-counted
// but the 
BOOST_AUTO_TEST_CASE(TestBidMultiLevelFillRestore)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 0, 1300);
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder bid0(true,  1251, 200);
  SimpleOrder bid1(true,  1250, 200);
  SimpleOrder bid2(true,  1250, 200);
  SimpleOrder bid3(true,  1248, 200);
  SimpleOrder bid4(true,  1247, 200);
  SimpleOrder bid5(true,  1246, 200);
  SimpleOrder bid6(true,  1245, 200); // Partial match
  SimpleOrder bid7(true,  1244, 200);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid4, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid5, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid6, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid7, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(8, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 2, 400));
  BOOST_REQUIRE(dc.verify_bid(1248, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1247, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1246, 1, 200));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc0(&bid0,  200,  250200);
    SimpleFillCheck fc1(&bid1,  200,  250000);
    SimpleFillCheck fc2(&bid2,  200,  250000);
    SimpleFillCheck fc3(&bid3,  200,  249600);
    SimpleFillCheck fc4(&bid4,  200,  249400);
    SimpleFillCheck fc5(&bid5,  200,  249200);
    SimpleFillCheck fc6(&bid6,  100,  124500);
    SimpleFillCheck fc7(&ask1, 1300, 1622900);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1245, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1244, 1, 200));
}

BOOST_AUTO_TEST_CASE(TestAskMultiLevelFillRestore)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false,  1251, 200); // Partial match
  SimpleOrder ask1(false,  1250, 200);
  SimpleOrder ask2(false,  1250, 300);
  SimpleOrder ask3(false,  1248, 200);
  SimpleOrder ask4(false,  1247, 200);
  SimpleOrder ask5(false,  1245, 200);
  SimpleOrder ask6(false,  1245, 200);
  SimpleOrder ask7(false,  1244, 200);
  SimpleOrder bid1(true, 0, 1550);
  SimpleOrder bid0(true, 1242, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask4, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask5, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask6, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask7, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(8, order_book.asks().size());
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1244, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1245, 2, 400));
  BOOST_REQUIRE(dc.verify_ask(1247, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1248, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1250, 2, 500));
  BOOST_REQUIRE(dc.verify_bid(1242, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc7(&ask7,  200,  248800);
    SimpleFillCheck fc6(&ask6,  200,  249000);
    SimpleFillCheck fc5(&ask5,  200,  249000);
    SimpleFillCheck fc4(&ask4,  200,  249400);
    SimpleFillCheck fc3(&ask3,  200,  249600);
    SimpleFillCheck fc2(&ask2,  300,  375000);
    SimpleFillCheck fc1(&ask1,  200,  250000);
    SimpleFillCheck fc0(&ask0,   50,   62550);
    SimpleFillCheck fc8(&bid1, 1550, 1933350);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 150));
  BOOST_REQUIRE(dc.verify_bid(1242, 1, 100));
}




} // namespace
