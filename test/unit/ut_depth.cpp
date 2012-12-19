#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE liquibook_Depth
#include <boost/test/unit_test.hpp>
#include "book/depth.h"
#include "changed_checker.h"
#include <iostream>

namespace liquibook {

using book::Depth;
using book::DepthLevel;
typedef Depth<5> SizedDepth;

bool verify_level(const DepthLevel*& level, 
                  Price price, 
                  uint32_t order_count, 
                  Quantity aggregate_qty)
{
  bool matched = true;
  if (price != level->price()) {
    std::cout << "Level price " << level->price() << std::endl;
    matched = false;
  }
  if (order_count != level->order_count()) {
    std::cout << "Level order count " << level->order_count() << std::endl;
    matched = false;
  }
  if (aggregate_qty != level->aggregate_qty()) {
    std::cout << "Level aggregate qty " << level->aggregate_qty() << std::endl;
    matched = false;
  }
  ++level;
  return matched;
}

BOOST_AUTO_TEST_CASE(TestAddBid)
{
  SizedDepth depth;
  depth.add_bid(1234, 100);
  const DepthLevel* first_bid = depth.bids();
  BOOST_REQUIRE(verify_level(first_bid, 1234, 1, 100));
  ChangedChecker cc(depth);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestAddBids)
{
  SizedDepth depth;
  depth.add_bid(1234, 100);
  depth.add_bid(1234, 200);
  depth.add_bid(1234, 300);
  const DepthLevel* first_bid = depth.bids();
  BOOST_REQUIRE(verify_level(first_bid, 1234, 3, 600));
  ChangedChecker cc(depth);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 0, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestAppendBidLevels)
{
  SizedDepth depth;
  depth.add_bid(1236, 300);
  depth.add_bid(1235, 200);
  depth.add_bid(1232, 100);
  depth.add_bid(1235, 400);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1, 300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2, 600));
  BOOST_REQUIRE(verify_level(bid, 1232, 1, 100));
  ChangedChecker cc(depth);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 4, 3, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestInsertBidLevels)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1234, 800);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1232, 100);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  depth.add_bid(1236, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 3, 3, 0, 0));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 4, 4, 4, 0));
  depth.add_bid(1234, 900);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 4, 5, 4, 0));
  depth.add_bid(1231, 700);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 4, 5, 4, 6));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 5, 4, 6));
  depth.add_bid(1231, 500);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 5, 4, 8));
  depth.add_bid(1233, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 5, 9, 9));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1,  300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(bid, 1234, 2, 1700));
  BOOST_REQUIRE(verify_level(bid, 1233, 1,  200));
  BOOST_REQUIRE(verify_level(bid, 1232, 1,  100));
}

BOOST_AUTO_TEST_CASE(TestInsertBidLevelsPast5)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1234, 800);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1232, 100);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  depth.add_bid(1236, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 3, 3, 0, 0));
  depth.add_bid(1231, 700);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 3, 3, 4, 0));
  depth.add_bid(1234, 900);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 5, 3, 4, 0));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 6, 6, 6, 6));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 6, 6, 6));
  depth.add_bid(1231, 500);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 6, 6, 8));
  depth.add_bid(1230, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 6, 6, 8));
  depth.add_bid(1229, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 6, 6, 8));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1,  300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(bid, 1234, 2, 1700));
  BOOST_REQUIRE(verify_level(bid, 1232, 1,  100));
  BOOST_REQUIRE(verify_level(bid, 1231, 2, 1200));
}

BOOST_AUTO_TEST_CASE(TestInsertBidLevelsTruncate5)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1234, 800);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1232, 100);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  depth.add_bid(1236, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 3, 3, 0, 0));
  depth.add_bid(1231, 700);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 3, 3, 4, 0));
  depth.add_bid(1234, 900);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 5, 3, 4, 0));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 6, 6, 6, 6));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 6, 6, 6));
  depth.add_bid(1231, 500);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 6, 6, 8));
  depth.add_bid(1230, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 6, 6, 8));
  depth.add_bid(1238, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(9, 9, 9, 9, 9));
  depth.add_bid(1238, 250);
  BOOST_REQUIRE(cc.verify_bid_stamps(10, 9, 9, 9, 9));
  depth.add_bid(1237, 500);
  BOOST_REQUIRE(cc.verify_bid_stamps(10, 11, 11, 11, 11));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1238, 2,  450));
  BOOST_REQUIRE(verify_level(bid, 1237, 1,  500));
  BOOST_REQUIRE(verify_level(bid, 1236, 1,  300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(bid, 1234, 2, 1700));
}

BOOST_AUTO_TEST_CASE(TestCloseBid)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1234, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1234, 500);
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 0, 0, 0, 0));
  BOOST_REQUIRE(!depth.close_bid(1234, 300)); // Does not erase
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 0, 0, 0, 0));
  const DepthLevel* first_bid = depth.bids();
  BOOST_REQUIRE(verify_level(first_bid, 1234, 1, 500));
}

BOOST_AUTO_TEST_CASE(TestCloseEraseBid)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1235, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 0, 0, 0, 0));
  depth.add_bid(1234, 500);
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 3, 0, 0, 0));
  depth.add_bid(1233, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 3, 4, 0, 0));
  BOOST_REQUIRE(!depth.close_bid(1235, 300)); // Does not erase
  BOOST_REQUIRE(cc.verify_bid_stamps(5, 3, 4, 0, 0));
  BOOST_REQUIRE(depth.close_bid(1235, 400)); // Erase
  BOOST_REQUIRE(cc.verify_bid_stamps(6, 6, 6, 0, 0));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1234, 1, 500));
  BOOST_REQUIRE(verify_level(bid, 1233, 1, 200));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddBid)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1234, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.close_bid(1234, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 0, 0, 0, 0));
  depth.add_bid(1233, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 0, 0, 0, 0));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1233, 1, 200));
  BOOST_REQUIRE(verify_level(bid, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddHigherBid)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1234, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.close_bid(1234, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(2, 0, 0, 0, 0));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 0, 0, 0, 0));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1235, 1, 200));
  BOOST_REQUIRE(verify_level(bid, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestCloseBidsFreeLevels)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1234, 800);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1232, 100);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  depth.add_bid(1236, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 3, 3, 0, 0));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 4, 4, 4, 0));
  depth.add_bid(1234, 900);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 4, 5, 4, 0));
  depth.add_bid(1231, 700);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 4, 5, 4, 6));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 5, 4, 6));
  depth.add_bid(1231, 500);
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 5, 4, 8));
  depth.close_bid(1234, 900); // No erase
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 9, 4, 8));
  depth.close_bid(1232, 100); // Erase
  BOOST_REQUIRE(cc.verify_bid_stamps(3, 7, 9, 10, 10));
  depth.close_bid(1236, 300); // Erase
  BOOST_REQUIRE(cc.verify_bid_stamps(11, 11, 11, 11, 10));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(bid, 1234, 1,  800));
  BOOST_REQUIRE(verify_level(bid, 1231, 2, 1200));
  BOOST_REQUIRE(verify_level(bid,    0, 0,    0));
  BOOST_REQUIRE(verify_level(bid,    0, 0,    0));
  depth.add_bid(1233, 350); // Insert
  BOOST_REQUIRE(cc.verify_bid_stamps(11, 11, 12, 12, 10));
  depth.add_bid(1236, 300); // Insert
  BOOST_REQUIRE(cc.verify_bid_stamps(13, 13, 13, 13, 13));
  depth.add_bid(1231, 700);
  BOOST_REQUIRE(cc.verify_bid_stamps(13, 13, 13, 13, 14));
  bid = depth.bids();  // reset
  BOOST_REQUIRE(verify_level(bid, 1236, 1,  300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(bid, 1234, 1,  800));
  BOOST_REQUIRE(verify_level(bid, 1233, 1,  350));
  BOOST_REQUIRE(verify_level(bid, 1231, 3, 1900));
}

BOOST_AUTO_TEST_CASE(TestIncreaseBid)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1236, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  depth.add_bid(1232, 100);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 3, 0, 0));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 4, 3, 0, 0));
  depth.change_qty_bid(1232, 37);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 4, 5, 0, 0));
  depth.change_qty_bid(1232, 41);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 4, 6, 0, 0));
  depth.change_qty_bid(1235, 201);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 7, 6, 0, 0));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1, 300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2, 801));
  BOOST_REQUIRE(verify_level(bid, 1232, 1, 178));
}

BOOST_AUTO_TEST_CASE(TestDecreaseBid)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1236, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  depth.add_bid(1232, 100);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 3, 0, 0));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 4, 3, 0, 0));
  depth.change_qty_bid(1236, -37);
  BOOST_REQUIRE(cc.verify_bid_stamps(5, 4, 3, 0, 0));
  depth.change_qty_bid(1236, -41);
  BOOST_REQUIRE(cc.verify_bid_stamps(6, 4, 3, 0, 0));
  depth.change_qty_bid(1235, -201);
  BOOST_REQUIRE(cc.verify_bid_stamps(6, 7, 3, 0, 0));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1, 222));
  BOOST_REQUIRE(verify_level(bid, 1235, 2, 399));
  BOOST_REQUIRE(verify_level(bid, 1232, 1, 100));
}

BOOST_AUTO_TEST_CASE(TestIncreaseDecreaseBid)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_bid(1236, 300);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 0, 0, 0, 0));
  depth.add_bid(1235, 200);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 0, 0, 0));
  depth.add_bid(1232, 100);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 2, 3, 0, 0));
  depth.add_bid(1235, 400);
  BOOST_REQUIRE(cc.verify_bid_stamps(1, 4, 3, 0, 0));
  depth.change_qty_bid(1236, 37);
  BOOST_REQUIRE(cc.verify_bid_stamps(5, 4, 3, 0, 0));
  depth.change_qty_bid(1235, -41);
  BOOST_REQUIRE(cc.verify_bid_stamps(5, 6, 3, 0, 0));
  depth.change_qty_bid(1232, 60);
  BOOST_REQUIRE(cc.verify_bid_stamps(5, 6, 7, 0, 0));
  depth.change_qty_bid(1236, -41);
  BOOST_REQUIRE(cc.verify_bid_stamps(8, 6, 7, 0, 0));
  depth.change_qty_bid(1236, 210);
  BOOST_REQUIRE(cc.verify_bid_stamps(9, 6, 7, 0, 0));
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1, 506));
  BOOST_REQUIRE(verify_level(bid, 1235, 2, 559));
  BOOST_REQUIRE(verify_level(bid, 1232, 1, 160));
}

BOOST_AUTO_TEST_CASE(TestAddAsk)
{
  SizedDepth depth;
  depth.add_ask(1234, 100);
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 1, 100));
  ChangedChecker cc(depth);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestAddAsks)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1234, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 0, 0, 0, 0));
  depth.add_ask(1234, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 0, 0, 0, 0));
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 3, 600));
}

BOOST_AUTO_TEST_CASE(TestAppendAskLevels)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 3, 3, 0, 0));
  depth.add_ask(1235, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 4, 3, 0, 0));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 100));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 600));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 300));
}

BOOST_AUTO_TEST_CASE(TestInsertAskLevels)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 800);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 3, 0, 0));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 4, 4, 0));
  depth.add_ask(1234, 900);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 5, 4, 4, 0));
  depth.add_ask(1231, 700);
  depth.add_ask(1235, 400);
  depth.add_ask(1231, 500);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1231, 2, 1200));
  BOOST_REQUIRE(verify_level(ask, 1232, 1,  100));
  BOOST_REQUIRE(verify_level(ask, 1234, 2, 1700));
  BOOST_REQUIRE(verify_level(ask, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(ask, 1236, 1,  300));
}

BOOST_AUTO_TEST_CASE(TestInsertAskLevelsPast5)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 800);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 3, 0, 0));
  depth.add_ask(1231, 700);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 4, 4, 0));
  depth.add_ask(1234, 900);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 5, 4, 0));
  depth.add_ask(1235, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 5, 6, 6));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 5, 7, 6));
  depth.add_ask(1231, 500);
  BOOST_REQUIRE(cc.verify_ask_stamps(8, 4, 5, 7, 6));
  depth.add_ask(1230, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(9, 9, 9, 9, 9));
  depth.add_ask(1229, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(10, 10, 10, 10, 10));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1229, 1,  200));
  BOOST_REQUIRE(verify_level(ask, 1230, 1,  200));
  BOOST_REQUIRE(verify_level(ask, 1231, 2, 1200));
  BOOST_REQUIRE(verify_level(ask, 1232, 1,  100));
  BOOST_REQUIRE(verify_level(ask, 1234, 2, 1700));
}

BOOST_AUTO_TEST_CASE(TestInsertAskLevelsTruncate5)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 800);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 3, 0, 0));
  depth.add_ask(1231, 700);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 4, 4, 0));
  depth.add_ask(1234, 900);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 5, 4, 0));
  depth.add_ask(1235, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 5, 6, 6));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 4, 5, 7, 6));
  depth.add_ask(1231, 500);
  BOOST_REQUIRE(cc.verify_ask_stamps(8, 4, 5, 7, 6));
  depth.add_ask(1230, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(9, 9, 9, 9, 9));
  depth.add_ask(1238, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(9, 9, 9, 9, 9));
  depth.add_ask(1232, 250);
  BOOST_REQUIRE(cc.verify_ask_stamps(9, 9, 10, 9, 9));
  depth.add_ask(1237, 500);
  BOOST_REQUIRE(cc.verify_ask_stamps(9, 9, 10, 9, 9));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1230, 1,  200));
  BOOST_REQUIRE(verify_level(ask, 1231, 2, 1200));
  BOOST_REQUIRE(verify_level(ask, 1232, 2,  350));
  BOOST_REQUIRE(verify_level(ask, 1234, 2, 1700));
  BOOST_REQUIRE(verify_level(ask, 1235, 2,  600));
}

BOOST_AUTO_TEST_CASE(TestCloseAsk)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1234, 500);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 0, 0, 0, 0));
  BOOST_REQUIRE(!depth.close_ask(1234, 300)); // Does not erase
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 1, 500));
}

BOOST_AUTO_TEST_CASE(TestCloseEraseAsk)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1233, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1234, 500);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 2, 0, 0, 0));
  depth.add_ask(1233, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 2, 0, 0, 0));
  BOOST_REQUIRE(!depth.close_ask(1233, 300)); // Does not erase
  BOOST_REQUIRE(cc.verify_ask_stamps(4, 2, 0, 0, 0));
  BOOST_REQUIRE(depth.close_ask(1233, 400)); // Erase
  BOOST_REQUIRE(cc.verify_ask_stamps(5, 5, 0, 0, 0));
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 1, 500));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddAsk)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.close_ask(1234, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 0, 0, 0, 0));
  depth.add_ask(1233, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 0, 0, 0, 0));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1233, 1, 200));
  BOOST_REQUIRE(verify_level(ask, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddHigherAsk)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.close_ask(1234, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 0, 0, 0, 0));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 0, 0, 0, 0));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1235, 1, 200));
  BOOST_REQUIRE(verify_level(ask, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestCloseAsksFreeLevels)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1234, 800);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 3, 0, 0));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 4, 4, 0));
  depth.add_ask(1234, 900);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 5, 4, 4, 0));
  depth.add_ask(1231, 700);
  BOOST_REQUIRE(cc.verify_ask_stamps(6, 6, 6, 6, 6));
  depth.add_ask(1235, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(6, 6, 6, 7, 6));
  depth.add_ask(1231, 500);
  BOOST_REQUIRE(cc.verify_ask_stamps(8, 6, 6, 7, 6));
  depth.close_ask(1234, 900); // does not erase
  BOOST_REQUIRE(cc.verify_ask_stamps(8, 6, 9, 7, 6));
  depth.close_ask(1232, 100); // erase
  BOOST_REQUIRE(cc.verify_ask_stamps(8, 10, 10, 10, 10));
  depth.close_ask(1236, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(8, 10, 10, 11, 10));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1231, 2, 1200));
  BOOST_REQUIRE(verify_level(ask, 1234, 1,  800));
  BOOST_REQUIRE(verify_level(ask, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(ask,    0, 0,    0));
  BOOST_REQUIRE(verify_level(ask,    0, 0,    0));
  depth.add_ask(1233, 350);
  depth.add_ask(1236, 300);
  depth.add_ask(1231, 700);
  ask = depth.asks();  // reset
  BOOST_REQUIRE(verify_level(ask, 1231, 3, 1900));
  BOOST_REQUIRE(verify_level(ask, 1233, 1,  350));
  BOOST_REQUIRE(verify_level(ask, 1234, 1,  800));
  BOOST_REQUIRE(verify_level(ask, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(ask, 1236, 1,  300));
}

BOOST_AUTO_TEST_CASE(TestIncreaseAsk)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 3, 3, 0, 0));
  depth.add_ask(1235, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 4, 3, 0, 0));
  depth.change_qty_ask(1232, 37);
  BOOST_REQUIRE(cc.verify_ask_stamps(5, 4, 3, 0, 0));
  depth.change_qty_ask(1232, 41);
  BOOST_REQUIRE(cc.verify_ask_stamps(6, 4, 3, 0, 0));
  depth.change_qty_ask(1235, 201);
  BOOST_REQUIRE(cc.verify_ask_stamps(6, 7, 3, 0, 0));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 178));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 801));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 300));
}

BOOST_AUTO_TEST_CASE(TestDecreaseAsk)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 3, 3, 0, 0));
  depth.add_ask(1235, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 4, 3, 0, 0));
  depth.change_qty_ask(1236, -37);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 4, 5, 0, 0));
  depth.change_qty_ask(1236, -41);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 4, 6, 0, 0));
  depth.change_qty_ask(1235, -201);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 7, 6, 0, 0));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 100));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 399));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 222));
}

BOOST_AUTO_TEST_CASE(TestIncreaseDecreaseAsk)
{
  SizedDepth depth;
  ChangedChecker cc(depth);
  depth.add_ask(1236, 300);
  BOOST_REQUIRE(cc.verify_ask_stamps(1, 0, 0, 0, 0));
  depth.add_ask(1235, 200);
  BOOST_REQUIRE(cc.verify_ask_stamps(2, 2, 0, 0, 0));
  depth.add_ask(1232, 100);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 3, 3, 0, 0));
  depth.add_ask(1235, 400);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 4, 3, 0, 0));
  depth.change_qty_ask(1236, 37);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 4, 5, 0, 0));
  depth.change_qty_ask(1235, -41);
  BOOST_REQUIRE(cc.verify_ask_stamps(3, 6, 5, 0, 0));
  depth.change_qty_ask(1232, 51);
  BOOST_REQUIRE(cc.verify_ask_stamps(7, 6, 5, 0, 0));
  depth.change_qty_ask(1236, -41);
  BOOST_REQUIRE(cc.verify_ask_stamps(7, 6, 8, 0, 0));
  depth.change_qty_ask(1236, 201);
  BOOST_REQUIRE(cc.verify_ask_stamps(7, 6, 9, 0, 0));
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 151));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 559));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 497));
}

} // namespace
