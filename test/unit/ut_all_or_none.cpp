// Copyright (c) 2012, 2013 Object Computing, Inc.
// All rights reserved.
// See the file license.txt for licensing information.
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE liquibook_AllOrNone
#include <boost/test/unit_test.hpp>
#include "ut_utils.h"

namespace liquibook {

using impl::SimpleOrder;
typedef FillCheck<SimpleOrder*> SimpleFillCheck;

OrderConditions AON(oc_all_or_none);

BOOST_AUTO_TEST_CASE(TestRegBidMatchAon)
{
  SimpleOrderBook order_book;
  SimpleOrder ask2(false, 1252, 100);
  SimpleOrder ask1(false, 1251, 100); // AON
  SimpleOrder ask0(false, 1251, 200); // AON, but skipped
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 2, 300));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 100, 125100);
    SimpleFillCheck fc2(&ask1, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestRegBidMatchMulti)
{
  SimpleOrderBook order_book;
  SimpleOrder ask2(false, 1251, 700);
  SimpleOrder ask1(false, 1251, 100); // AON
  SimpleOrder ask0(false, 1251, 100); // AON
  SimpleOrder bid1(true,  1251, 400);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 3, 900));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc0(&bid1, 400, 1251 * 400);
    SimpleFillCheck fc1(&ask0, 100, 1251 * 100);
    SimpleFillCheck fc2(&ask1, 100, 1251 * 100);
    SimpleFillCheck fc3(&ask2, 200, 1251 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 500));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonBidNoMatch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid1(true,  1251, 300); // AON
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
    SimpleFillCheck fc1(&bid1, 0, 0);
    SimpleFillCheck fc2(&ask0, 0, 0);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 300));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonBidMatchReg)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 400);
  SimpleOrder bid1(true,  1251, 300); // AON
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
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 300, 1251 * 300);
    SimpleFillCheck fc2(&ask0, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonBidMatchMulti)
{
  SimpleOrderBook order_book;
  SimpleOrder ask3(false, 1252, 100);
  SimpleOrder ask2(false, 1252, 100);
  SimpleOrder ask1(false, 1251, 400); // AON no match
  SimpleOrder ask0(false, 1251, 400);
  SimpleOrder bid1(true,  0,    600); // AON
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask3, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(4, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 2, 800));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 200));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 600, 750800);
    SimpleFillCheck fc2(&ask0, 400, 1251 * 400);
    SimpleFillCheck fc3(&ask2, 100, 1252 * 100);
    SimpleFillCheck fc4(&ask3, 100, 1252 * 100);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 400));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonBidNoMatchMulti)
{
  SimpleOrderBook order_book;
  SimpleOrder ask2(false, 1252, 400); // AON no match
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 400);
  SimpleOrder bid1(true,  0,    600); // AON
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false, false, AON));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 500));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 0, 0);
    SimpleFillCheck fc2(&ask0, 0, 0);
    SimpleFillCheck fc3(&ask1, 0, 0);
    SimpleFillCheck fc4(&ask2, 0, 0);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 2, 500));
}

BOOST_AUTO_TEST_CASE(TestAonBidMatchAon)
{
  SimpleOrderBook order_book;
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 300); // AON
  SimpleOrder bid1(true,  1251, 300); // AON
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 300));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 300, 1251 * 300);
    SimpleFillCheck fc2(&ask0, 300, 1251 * 300);
    BOOST_REQUIRE(add_and_verify(order_book, &bid1, true, true, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestRegAskMatchAon)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false, 1251, 100);
  SimpleOrder bid1(true,  1251, 200); // AON, but skipped
  SimpleOrder bid2(true,  1251, 100); // AON
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 300));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid2, 100, 125100);
    SimpleFillCheck fc2(&ask1, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestRegAskMatchMulti)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false, 1250, 400);
  SimpleOrder bid1(true,  1251, 100); // AON
  SimpleOrder bid2(true,  1251, 100); // AON
  SimpleOrder bid0(true,  1250, 700);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false, false, AON));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc0(&bid1, 100, 1251 * 100);
    SimpleFillCheck fc1(&bid2, 100, 1251 * 100);
    SimpleFillCheck fc2(&bid0, 200, 1250 * 200);
    SimpleFillCheck fc3(&ask1, 400, 500200);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 500));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonAskNoMatch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false, 1251, 400); // AON
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid2(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 700);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc0(&bid1, 0, 0);
    SimpleFillCheck fc1(&bid2, 0, 0);
    SimpleFillCheck fc2(&bid0, 0, 0);
    SimpleFillCheck fc3(&ask1, 0, 0);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, false, false, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonAskMatchReg)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false, 1251, 100); // AON
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 700);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc0(&bid1, 100, 125100);
    SimpleFillCheck fc3(&ask1, 100, 125100);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonAskMatchMulti)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false, 1250, 600); // AON
  SimpleOrder bid1(true,  1251, 100); // AON
  SimpleOrder bid2(true,  1251, 100);
  SimpleOrder bid3(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 700);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid3, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(4, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 3, 300));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 700));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc0(&bid1, 100, 125100);
    SimpleFillCheck fc1(&bid2, 100, 125100);
    SimpleFillCheck fc2(&bid3, 100, 125100);
    SimpleFillCheck fc3(&bid0, 300, 1250 * 300);
    SimpleFillCheck fc4(&ask1, 600, 750300);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonAskNoMatchMulti)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false, 1250, 600); // AON
  SimpleOrder bid1(true,  1251, 100); // AON
  SimpleOrder bid2(true,  1251, 400);
  SimpleOrder bid0(true,  1250, 400); // AON no match

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &bid2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 500));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // No match
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc0(&bid0, 0, 0);
    SimpleFillCheck fc1(&bid1, 0, 0);
    SimpleFillCheck fc2(&bid2, 0, 0);
    SimpleFillCheck fc3(&ask1, 0, 0);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, false, false, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1251, 2, 500));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 400));
  BOOST_REQUIRE(dc.verify_ask(1250, 1, 600));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(3, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestAonAskMatchAon)
{
  SimpleOrderBook order_book;
  SimpleOrder ask0(false, 1252, 100);
  SimpleOrder ask1(false, 1251, 200); // AON
  SimpleOrder bid1(true,  1251, 200); // AON
  SimpleOrder bid0(true,  1250, 400);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 400));

  // Match complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&bid1, 200, 1251 * 200);
    SimpleFillCheck fc3(&ask1, 200, 1251 * 200);
    BOOST_REQUIRE(add_and_verify(order_book, &ask1, true, true, AON));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 400));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestReplaceAonBidSmallerMatch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask2(false, 1253, 100);
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid1(true,  1251, 200); // AON
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc2(&ask0, 100, 125100);
    BOOST_REQUIRE(replace_and_verify(
        order_book, &bid1, -100, PRICE_UNCHANGED, impl::os_complete, 100));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestReplaceAonBidPriceMatch)
{
  SimpleOrderBook order_book;
  SimpleOrder ask2(false, 1253, 100);
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 100);
  SimpleOrder bid1(true,  1251, 200); // AON
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc1(&ask0, 100, 125100);
    SimpleFillCheck fc2(&ask1, 100, 125200);
    BOOST_REQUIRE(replace_and_verify(
        order_book, &bid1, 0, 1252, impl::os_complete, 200));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(1, order_book.asks().size());
}

BOOST_AUTO_TEST_CASE(TestReplaceBidLargerMatchAon)
{
  SimpleOrderBook order_book;
  SimpleOrder ask2(false, 1253, 100);
  SimpleOrder ask1(false, 1252, 100);
  SimpleOrder ask0(false, 1251, 200); // AON
  SimpleOrder bid1(true,  1251, 100);
  SimpleOrder bid0(true,  1250, 100);

  // No match
  BOOST_REQUIRE(add_and_verify(order_book, &bid0, false));
  BOOST_REQUIRE(add_and_verify(order_book, &bid1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask0, false, false, AON));
  BOOST_REQUIRE(add_and_verify(order_book, &ask1, false));
  BOOST_REQUIRE(add_and_verify(order_book, &ask2, false));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(2, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(3, order_book.asks().size());

  // Verify depth
  DepthCheck dc(order_book.depth());
  BOOST_REQUIRE(dc.verify_bid(1251, 1, 100));
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1251, 1, 200));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 100));

  // Match - complete
  { BOOST_REQUIRE_NO_THROW(
    SimpleFillCheck fc2(&ask0, 200, 200 * 1251);
    BOOST_REQUIRE(replace_and_verify(
        order_book, &bid1, 100, PRICE_UNCHANGED, impl::os_complete, 200));
  ); }

  // Verify depth
  dc.reset();
  BOOST_REQUIRE(dc.verify_bid(1250, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1252, 1, 100));
  BOOST_REQUIRE(dc.verify_ask(1253, 1, 100));

  // Verify sizes
  BOOST_REQUIRE_EQUAL(1, order_book.bids().size());
  BOOST_REQUIRE_EQUAL(2, order_book.asks().size());
}

} // Namespace
