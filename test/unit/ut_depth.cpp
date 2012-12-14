#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE liquibook_Depth
#include <boost/test/unit_test.hpp>
#include "book/depth.h"

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
}

BOOST_AUTO_TEST_CASE(TestAddBids)
{
  SizedDepth depth;
  depth.add_bid(1234, 100);
  depth.add_bid(1234, 200);
  depth.add_bid(1234, 300);
  const DepthLevel* first_bid = depth.bids();
  BOOST_REQUIRE(verify_level(first_bid, 1234, 3, 600));
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
}

BOOST_AUTO_TEST_CASE(TestInsertBidLevels)
{
  SizedDepth depth;
  depth.add_bid(1234, 800);
  depth.add_bid(1232, 100);
  depth.add_bid(1236, 300);
  depth.add_bid(1235, 200);
  depth.add_bid(1234, 900);
  depth.add_bid(1231, 700);
  depth.add_bid(1235, 400);
  depth.add_bid(1231, 500);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1,  300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(bid, 1234, 2, 1700));
  BOOST_REQUIRE(verify_level(bid, 1232, 1,  100));
  BOOST_REQUIRE(verify_level(bid, 1231, 2, 1200));
}

BOOST_AUTO_TEST_CASE(TestInsertBidLevelsPast5)
{
  SizedDepth depth;
  depth.add_bid(1234, 800);
  depth.add_bid(1232, 100);
  depth.add_bid(1236, 300);
  depth.add_bid(1231, 700);
  depth.add_bid(1234, 900);
  depth.add_bid(1235, 400);
  depth.add_bid(1235, 200);
  depth.add_bid(1231, 500);
  depth.add_bid(1230, 200);
  depth.add_bid(1229, 200);
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
  depth.add_bid(1234, 800);
  depth.add_bid(1232, 100);
  depth.add_bid(1236, 300);
  depth.add_bid(1231, 700);
  depth.add_bid(1234, 900);
  depth.add_bid(1235, 400);
  depth.add_bid(1235, 200);
  depth.add_bid(1231, 500);
  depth.add_bid(1230, 200);
  depth.add_bid(1238, 200);
  depth.add_bid(1238, 250);
  depth.add_bid(1237, 500);
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
  depth.add_bid(1234, 300);
  depth.add_bid(1234, 500);
  BOOST_REQUIRE(!depth.close_bid(1234, 300)); // Does not erase
  const DepthLevel* first_bid = depth.bids();
  BOOST_REQUIRE(verify_level(first_bid, 1234, 1, 500));
}

BOOST_AUTO_TEST_CASE(TestCloseEraseBid)
{
  SizedDepth depth;
  depth.add_bid(1235, 300);
  depth.add_bid(1235, 400);
  depth.add_bid(1234, 500);
  BOOST_REQUIRE(!depth.close_bid(1235, 300)); // Does not erase
  BOOST_REQUIRE(depth.close_bid(1235, 400)); // Erase
  const DepthLevel* first_bid = depth.bids();
  BOOST_REQUIRE(verify_level(first_bid, 1234, 1, 500));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddBid)
{
  SizedDepth depth;
  depth.add_bid(1234, 300);
  depth.close_bid(1234, 300);
  depth.add_bid(1233, 200);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1233, 1, 200));
  BOOST_REQUIRE(verify_level(bid, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddHigherBid)
{
  SizedDepth depth;
  depth.add_bid(1234, 300);
  depth.close_bid(1234, 300);
  depth.add_bid(1235, 200);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1235, 1, 200));
  BOOST_REQUIRE(verify_level(bid, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestCloseBidsFreeLevels)
{
  SizedDepth depth;
  depth.add_bid(1234, 800);
  depth.add_bid(1232, 100);
  depth.add_bid(1236, 300);
  depth.add_bid(1235, 200);
  depth.add_bid(1234, 900);
  depth.add_bid(1231, 700);
  depth.add_bid(1235, 400);
  depth.add_bid(1231, 500);
  depth.close_bid(1234, 900);
  depth.close_bid(1232, 100);
  depth.close_bid(1236, 100);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1235, 2,  600));
  BOOST_REQUIRE(verify_level(bid, 1234, 1,  800));
  BOOST_REQUIRE(verify_level(bid, 1231, 2, 1200));
  BOOST_REQUIRE(verify_level(bid,    0, 0,    0));
  BOOST_REQUIRE(verify_level(bid,    0, 0,    0));
  depth.add_bid(1233, 350);
  depth.add_bid(1236, 300);
  depth.add_bid(1231, 700);
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
  depth.add_bid(1236, 300);
  depth.add_bid(1235, 200);
  depth.add_bid(1232, 100);
  depth.add_bid(1235, 400);
  depth.increase_bid(1232, 37);
  depth.increase_bid(1232, 41);
  depth.increase_bid(1235, 201);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1, 300));
  BOOST_REQUIRE(verify_level(bid, 1235, 2, 801));
  BOOST_REQUIRE(verify_level(bid, 1232, 1, 178));
}

BOOST_AUTO_TEST_CASE(TestDecreaseBid)
{
  SizedDepth depth;
  depth.add_bid(1236, 300);
  depth.add_bid(1235, 200);
  depth.add_bid(1232, 100);
  depth.add_bid(1235, 400);
  depth.decrease_bid(1236, 37);
  depth.decrease_bid(1236, 41);
  depth.decrease_bid(1235, 201);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1, 222));
  BOOST_REQUIRE(verify_level(bid, 1235, 2, 399));
  BOOST_REQUIRE(verify_level(bid, 1232, 1, 100));
}

BOOST_AUTO_TEST_CASE(TestIncreaseDecreaseBid)
{
  SizedDepth depth;
  depth.add_bid(1236, 300);
  depth.add_bid(1235, 200);
  depth.add_bid(1232, 100);
  depth.add_bid(1235, 400);
  depth.increase_bid(1236, 37);
  depth.decrease_bid(1235, 41);
  depth.increase_bid(1232, 51);
  depth.decrease_bid(1236, 41);
  depth.increase_bid(1236, 201);
  const DepthLevel* bid = depth.bids();
  BOOST_REQUIRE(verify_level(bid, 1236, 1, 497));
  BOOST_REQUIRE(verify_level(bid, 1235, 2, 559));
  BOOST_REQUIRE(verify_level(bid, 1232, 1, 151));
}

BOOST_AUTO_TEST_CASE(TestAddAsk)
{
  SizedDepth depth;
  depth.add_ask(1234, 100);
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 1, 100));
}

BOOST_AUTO_TEST_CASE(TestAddAsks)
{
  SizedDepth depth;
  depth.add_ask(1234, 100);
  depth.add_ask(1234, 200);
  depth.add_ask(1234, 300);
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 3, 600));
}

BOOST_AUTO_TEST_CASE(TestAppendAskLevels)
{
  SizedDepth depth;
  depth.add_ask(1236, 300);
  depth.add_ask(1235, 200);
  depth.add_ask(1232, 100);
  depth.add_ask(1235, 400);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 100));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 600));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 300));
}

BOOST_AUTO_TEST_CASE(TestInsertAskLevels)
{
  SizedDepth depth;
  depth.add_ask(1234, 800);
  depth.add_ask(1232, 100);
  depth.add_ask(1236, 300);
  depth.add_ask(1235, 200);
  depth.add_ask(1234, 900);
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
  depth.add_ask(1234, 800);
  depth.add_ask(1232, 100);
  depth.add_ask(1236, 300);
  depth.add_ask(1231, 700);
  depth.add_ask(1234, 900);
  depth.add_ask(1235, 400);
  depth.add_ask(1235, 200);
  depth.add_ask(1231, 500);
  depth.add_ask(1230, 200);
  depth.add_ask(1229, 200);
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
  depth.add_ask(1234, 800);
  depth.add_ask(1232, 100);
  depth.add_ask(1236, 300);
  depth.add_ask(1231, 700);
  depth.add_ask(1234, 900);
  depth.add_ask(1235, 400);
  depth.add_ask(1235, 200);
  depth.add_ask(1231, 500);
  depth.add_ask(1230, 200);
  depth.add_ask(1238, 200);
  depth.add_ask(1238, 250);
  depth.add_ask(1237, 500);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1230, 1,  200));
  BOOST_REQUIRE(verify_level(ask, 1231, 2, 1200));
  BOOST_REQUIRE(verify_level(ask, 1232, 1,  100));
  BOOST_REQUIRE(verify_level(ask, 1234, 2, 1700));
  BOOST_REQUIRE(verify_level(ask, 1235, 2,  600));
}

BOOST_AUTO_TEST_CASE(TestCloseAsk)
{
  SizedDepth depth;
  depth.add_ask(1234, 300);
  depth.add_ask(1234, 500);
  BOOST_REQUIRE(!depth.close_ask(1234, 300)); // Does not erase
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 1, 500));
}

BOOST_AUTO_TEST_CASE(TestCloseEraseAsk)
{
  SizedDepth depth;
  depth.add_ask(1233, 300);
  depth.add_ask(1234, 500);
  depth.add_ask(1233, 400);
  BOOST_REQUIRE(!depth.close_ask(1233, 300)); // Does not erase
  BOOST_REQUIRE(depth.close_ask(1233, 400)); // Erase
  const DepthLevel* first_ask = depth.asks();
  BOOST_REQUIRE(verify_level(first_ask, 1234, 1, 500));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddAsk)
{
  SizedDepth depth;
  depth.add_ask(1234, 300);
  depth.close_ask(1234, 300);
  depth.add_ask(1233, 200);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1233, 1, 200));
  BOOST_REQUIRE(verify_level(ask, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestAddCloseAddHigherAsk)
{
  SizedDepth depth;
  depth.add_ask(1234, 300);
  depth.close_ask(1234, 300);
  depth.add_ask(1235, 200);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1235, 1, 200));
  BOOST_REQUIRE(verify_level(ask, 0, 0, 0));
}

BOOST_AUTO_TEST_CASE(TestCloseAsksFreeLevels)
{
  SizedDepth depth;
  depth.add_ask(1234, 800);
  depth.add_ask(1232, 100);
  depth.add_ask(1236, 300);
  depth.add_ask(1235, 200);
  depth.add_ask(1234, 900);
  depth.add_ask(1231, 700);
  depth.add_ask(1235, 400);
  depth.add_ask(1231, 500);
  depth.close_ask(1234, 900);
  depth.close_ask(1232, 100);
  depth.close_ask(1236, 100);
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
  depth.add_ask(1236, 300);
  depth.add_ask(1235, 200);
  depth.add_ask(1232, 100);
  depth.add_ask(1235, 400);
  depth.increase_ask(1232, 37);
  depth.increase_ask(1232, 41);
  depth.increase_ask(1235, 201);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 178));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 801));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 300));
}

BOOST_AUTO_TEST_CASE(TestDecreaseAsk)
{
  SizedDepth depth;
  depth.add_ask(1236, 300);
  depth.add_ask(1235, 200);
  depth.add_ask(1232, 100);
  depth.add_ask(1235, 400);
  depth.decrease_ask(1236, 37);
  depth.decrease_ask(1236, 41);
  depth.decrease_ask(1235, 201);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 100));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 399));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 222));
}

BOOST_AUTO_TEST_CASE(TestIncreaseDecreaseAsk)
{
  SizedDepth depth;
  depth.add_ask(1236, 300);
  depth.add_ask(1235, 200);
  depth.add_ask(1232, 100);
  depth.add_ask(1235, 400);
  depth.increase_ask(1236, 37);
  depth.decrease_ask(1235, 41);
  depth.increase_ask(1232, 51);
  depth.decrease_ask(1236, 41);
  depth.increase_ask(1236, 201);
  const DepthLevel* ask = depth.asks();
  BOOST_REQUIRE(verify_level(ask, 1232, 1, 151));
  BOOST_REQUIRE(verify_level(ask, 1235, 2, 559));
  BOOST_REQUIRE(verify_level(ask, 1236, 1, 497));
}

} // namespace
