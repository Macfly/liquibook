#include "book/depth.h"
#include <iostream>

namespace liquibook {

using book::Depth;
using book::DepthLevel;

namespace test {

template <int SIZE = 5>
class ChangedChecker {
public:
  typedef Depth<SIZE> SizedDepth;
  ChangedChecker(const SizedDepth& depth)
  : depth_(depth)
  {
  }

  bool verify_bid_stamps(ChangeId l0, ChangeId l1, ChangeId l2, 
                  ChangeId l3, ChangeId l4)
  {
    return verify_side_stamps(depth_.bids(), l0, l1, l2, l3, l4);
  }

  bool verify_ask_stamps(ChangeId l0, ChangeId l1, ChangeId l2, 
                  ChangeId l3, ChangeId l4)
  {
    return verify_side_stamps(depth_.asks(), l0, l1, l2, l3, l4);
  }

  bool verify_bbo_stamps(ChangeId bid_stamp, ChangeId ask_stamp)
  {
    bool matched = true;
    if (depth_.bids()->last_change() != bid_stamp) {
      std::cout << "best bid change " 
                << depth_.bids()->last_change() << std::endl;
      matched = false;
    }
    if (depth_.asks()->last_change() != ask_stamp) {
      std::cout << "best ask change " 
                << depth_.asks()->last_change() << std::endl;
      matched = false;
    }
    return matched;
  }
  private:
  bool verify_side_stamps(const DepthLevel* start, 
                   ChangeId l0, ChangeId l1, ChangeId l2, 
                   ChangeId l3, ChangeId l4)
  {
    bool matched = true;
    if (start[0].last_change() != l0) {
      std::cout << "change id[0] " << start[0].last_change() << std::endl;
      matched = false;
    }
    if (start[1].last_change() != l1) {
      std::cout << "change id[1] " << start[1].last_change() << std::endl;
      matched = false;
    }
    if (start[2].last_change() != l2) {
      std::cout << "change id[2] " << start[2].last_change() << std::endl;
      matched = false;
    }
    if (start[3].last_change() != l3) {
      std::cout << "change id[3] " << start[3].last_change() << std::endl;
      matched = false;
    }
    if (start[4].last_change() != l4) {
      std::cout << "change id[4] " << start[4].last_change() << std::endl;
      matched = false;
    }
    return matched;
  }
  private:
  const SizedDepth& depth_;
};

} } // namespace
