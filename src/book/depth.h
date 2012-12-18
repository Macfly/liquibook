#ifndef depth_h
#define depth_h

#include "liquibook_book_export.h"
#include "depth_level.h"
#include "base/types.h"
#include <cmath>

namespace liquibook { namespace book {

template <int SIZE=5> 
class LIQUIBOOK_BOOK_Export Depth {
public:
  /// @brief construct
  Depth();

  /// @brief get the first bid level
  const DepthLevel* bids() const;
  /// @brief get the last bid level
  const DepthLevel* last_bid_level() const;
  /// @brief get the first ask level
  const DepthLevel* asks() const;
  /// @brief get the last ask level
  const DepthLevel* last_ask_level() const;
  /// @brief get one past the last ask level
  const DepthLevel* end() const;

  /// @brief get the first bid level
  DepthLevel* bids();
  /// @brief get the last bid level
  DepthLevel* last_bid_level();
  /// @brief get the first ask level
  DepthLevel* asks();
  /// @brief get the last ask level
  DepthLevel* last_ask_level();

  /// @brief add a bid order
  /// @param price the price level of the bid
  /// @param qty the open quantity of the bid
  void add_bid(Price price, Quantity qty);
  /// @brief cancel or fill a bid order
  /// @param price the price level of the bid
  /// @param qty the open quantity of the bid
  /// @return true if the close erased a visible level
  bool close_bid(Price price, Quantity qty);

  /// @brief change quantity of a bid order
  /// @param price the price level of the bid
  /// @param qty_delta the change in open quantity of the bid (+ or -)
  void change_qty_bid(Price price, int32_t qty_delta);
  
  // TODO REMOVE
  /// @brief increase size of a bid order
  /// @param price the price level of the bid
  /// @param qty_increase the change in open quantity of the bid
  void increase_bid(Price price, Quantity qty_increase);

  // TODO REMOVE
  /// @brief decrease size of a bid order
  /// @param price the price level of the bid
  /// @param qty_decrease the change in open quantity of the bid
  void decrease_bid(Price price, Quantity qty_decrease);

  /// @brief add a ask order
  /// @param price the price level of the ask
  /// @param qty the open quantity of the ask
  void add_ask(Price price, Quantity qty);
  /// @brief cancel or fill a ask order
  /// @param price the price level of the ask
  /// @param qty the open quantity of the ask
  /// @return true if the close erased a visible level
  bool close_ask(Price price, Quantity qty);

  /// @brief change quantity of a ask order
  /// @param price the price level of the ask
  /// @param qty_delta the change in open quantity of the ask (+ or -)
  void change_qty_ask(Price price, int32_t qty_delta);

  // TODO REMOVE
  /// @brief increase size of a ask order
  /// @param price the price level of the ask
  /// @param qty_increase the change in open quantity of the ask
  void increase_ask(Price price, Quantity qty_increase);

  // TODO REMOVE
  /// @brief decrease size of a ask order
  /// @param price the price level of the ask
  /// @param qty_decrease the change in open quantity of the ask
  void decrease_ask(Price price, Quantity qty_decrease);

  /// @brief does this depth need bid restoration after level erasure
  /// @param restoration_price the price to restore after (out)
  /// @return true if restoration is needed (previously was full)
  bool needs_bid_restoration(Price& restoration_price);

  /// @brief does this depth need ask restoration after level erasure
  /// @param restoration_price the price to restore after (out)
  /// @return true if restoration is needed (previously was full)
  bool needs_ask_restoration(Price& restoration_price);

  /// @brief has the depth changed since the last publish
  bool changed();

  /// @brief what was the last published change?
  ChangeId last_published_change();
private:
  DepthLevel levels_[SIZE*2];
  ChangeId last_change_;
  ChangeId last_published_change_;

  /// @brief find the level associated with the bid price
  /// @param price the price to find
  /// @param should_create should a level for the price be created, if necessary
  /// @return the level, or NULL if not found and full
  DepthLevel* find_bid(Price price, bool should_create = true);

  /// @brief find the level associated with the ask price
  /// @param price the price to find
  /// @param should_create should a level for the price be created, if necessary
  /// @return the level, or NULL if not found and full
  DepthLevel* find_ask(Price price, bool should_create = true);

  /// @brief insert a new level before this level and shift down
  /// @param level the level to insert before
  /// @param last_level the last level of the bids or asks
  /// @param price the price to initialize the level at
  void insert_level_before(DepthLevel* level,
                           DepthLevel* last_level,
                           Price price);

  /// @brief erase a level and shift up
  /// @param level the level to erase
  /// @param last_level the last level of the bids or asks
  void erase_level(DepthLevel* level, DepthLevel* last_level);
};

template <int SIZE> 
Depth<SIZE>::Depth()
: last_change_(0),
  last_published_change_(0)
{
  memset(levels_, 0, sizeof(DepthLevel) * SIZE * 2);
}

template <int SIZE> 
inline const DepthLevel* 
Depth<SIZE>::bids() const
{
  return levels_;
}

template <int SIZE> 
inline const DepthLevel* 
Depth<SIZE>::asks() const
{
  return levels_ + SIZE;
}

template <int SIZE> 
inline const DepthLevel*
Depth<SIZE>::last_bid_level() const
{
  return levels_ + (SIZE - 1);
}

template <int SIZE> 
inline const DepthLevel*
Depth<SIZE>::last_ask_level() const
{
  return levels_ + (SIZE * 2 - 1);
}

template <int SIZE> 
inline const DepthLevel* 
Depth<SIZE>::end() const
{
  return levels_ + (SIZE * 2);
}

template <int SIZE> 
inline DepthLevel* 
Depth<SIZE>::bids()
{
  return levels_;
}

template <int SIZE> 
inline DepthLevel* 
Depth<SIZE>::asks()
{
  return levels_ + SIZE;
}

template <int SIZE> 
inline DepthLevel*
Depth<SIZE>::last_bid_level()
{
  return levels_ + (SIZE - 1);
}

template <int SIZE> 
inline DepthLevel*
Depth<SIZE>::last_ask_level()
{
  return levels_ + (SIZE * 2 - 1);
}


template <int SIZE> 
inline void
Depth<SIZE>::add_bid(Price price, Quantity qty)
{
  ChangeId last_change_copy = last_change_;
  DepthLevel* level = find_bid(price);
  if (level) {
    last_change_ = last_change_copy + 1; // Ensure incremented
    level->add_order(qty);
    level->last_change(last_change_);
  }
}

template <int SIZE> 
inline bool
Depth<SIZE>::close_bid(Price price, Quantity qty)
{
  DepthLevel* level = find_bid(price, false);
  if (level) {
    if (level->close_order(qty)) {
      erase_level(level, last_bid_level());
      return true;  // Level erased
    } else {
      level->last_change(++last_change_);
    }
  }
  return false;
}

template <int SIZE> 
inline void
Depth<SIZE>::change_qty_bid(Price price, int32_t qty_delta)
{
  DepthLevel* level = find_bid(price, false);
  if (level && qty_delta) {
    if (qty_delta > 0) {
      level->increase_qty(Quantity(qty_delta));
    } else {
      level->decrease_qty(Quantity(std::abs(qty_delta)));
    }
    level->last_change(++last_change_);
  }
  // Ignore if not found - may be beyond our depth size
}
  
template <int SIZE> 
inline void
Depth<SIZE>::increase_bid(Price price, Quantity qty_increase)
{
  DepthLevel* level = find_bid(price, false);
  if (level) {
    level->increase_qty(qty_increase);
    level->last_change(++last_change_);
  }
  // Ignore if not found - may be beyond our depth size
}

template <int SIZE> 
inline void
Depth<SIZE>::decrease_bid(Price price, Quantity qty_decrease)
{
  DepthLevel* level = find_bid(price, false);
  if (level) {
    level->decrease_qty(qty_decrease);
    level->last_change(++last_change_);
  }
  // Ignore if not found - may be beyond our depth size
}

template <int SIZE> 
inline void
Depth<SIZE>::add_ask(Price price, Quantity qty)
{
  ChangeId last_change_copy = last_change_;
  DepthLevel* level = find_ask(price);
  if (level) {
    last_change_ = last_change_copy + 1; // Ensure incremented
    level->add_order(qty);
    level->last_change(last_change_);
  }
}

template <int SIZE> 
inline bool
Depth<SIZE>::close_ask(Price price, Quantity qty)
{
  DepthLevel* level = find_ask(price, false);
  if (level) {
    if (level->close_order(qty)) {
      erase_level(level, last_ask_level());
      return true;  // Level erased
    } else {
      level->last_change(++last_change_);
    }
  }
  return false;
}

template <int SIZE> 
inline void
Depth<SIZE>::change_qty_ask(Price price, int32_t qty_delta)
{
  DepthLevel* level = find_ask(price, false);
  if (level && qty_delta) {
    if (qty_delta > 0) {
      level->increase_qty(Quantity(qty_delta));
    } else {
      level->decrease_qty(Quantity(std::abs(qty_delta)));
    }
    level->last_change(++last_change_);
  }
  // Ignore if not found - may be beyond our depth size
}
  
template <int SIZE> 
inline void
Depth<SIZE>::increase_ask(Price price, Quantity qty_increase)
{
  DepthLevel* level = find_ask(price, false);
  if (level) {
    level->increase_qty(qty_increase);
    level->last_change(++last_change_);
  }
  // Ignore if not found - may be beyond our depth size
}

template <int SIZE> 
inline void
Depth<SIZE>::decrease_ask(Price price, Quantity qty_decrease)
{
  DepthLevel* level = find_ask(price, false);
  if (level) {
    level->decrease_qty(qty_decrease);
    level->last_change(++last_change_);
  }
  // Ignore if not found - may be beyond our depth size
}

template <int SIZE> 
inline bool
Depth<SIZE>::needs_bid_restoration(Price& restoration_price)
{
  // If this depth has multiple levels
  if (SIZE > 1) {
    // Restore using the price before the last level
    restoration_price = (last_bid_level() - 1)->price();
    // Restore if that level was valid
    return restoration_price != INVALID_LEVEL_PRICE;
  // Else this depth is BBO only
  } else if (SIZE == 1) {
    // There is no earlier level to look at, restore using the first non-market
    // bid price
    restoration_price = MARKET_ORDER_BID_SORT_PRICE;
    // Always restore on BBO only
    return true;
  }
  throw std::runtime_error("Depth size less than one not allowed");
}

template <int SIZE> 
inline bool
Depth<SIZE>::needs_ask_restoration(Price& restoration_price)
{
  // If this depth has multiple levels
  if (SIZE > 1) {
    // Restore using the price before the last level
    restoration_price = (last_ask_level() - 1)->price();
    // Restore if that level was valid
    return restoration_price != INVALID_LEVEL_PRICE;
  // Else this depth is BBO only
  } else if (SIZE == 1) {
    // There is no earlier level to look at, restore the first non-market
    // ask price
    restoration_price =  MARKET_ORDER_ASK_SORT_PRICE;
    // Always restore on BBO only
    return true;
  }
  throw std::runtime_error("Depth size less than one not allowed");
}

template <int SIZE> 
DepthLevel*
Depth<SIZE>::find_bid(Price price, bool should_create)
{
  DepthLevel* past_end = asks();
  DepthLevel* bid;
  // Linear search each bid
  for (bid = bids(); bid != past_end; ++bid) {
    // If the level price is correct
    if (bid->price() == price) {
      break;
    // Else if the level is blank
    } else if (should_create && bid->price() == INVALID_LEVEL_PRICE) {
      bid->init(price);  // Change ID will be assigned by caller
      break;  // Blank slot
    // Else if the level price is too low
    } else if (should_create && bid->price() < price) {
      // Insert a slot
      insert_level_before(bid, last_bid_level(), price);
      break;
    }
  }
  if (bid == past_end) {
    return NULL;
  }
  return bid;
}

template <int SIZE> 
DepthLevel*
Depth<SIZE>::find_ask(Price price, bool should_create)
{
  const DepthLevel* past_end = end();
  DepthLevel* ask;
  // Linear search each ask
  for (ask = asks(); ask != past_end; ++ask) {
    // If the level price is correct
    if (ask->price() == price) {
      break;
    // Else if the level is blank
    } else if (should_create && ask->price() == INVALID_LEVEL_PRICE) {
      ask->init(price);  // Change ID will be assigned by caller
      break;  // Blank slot
    // Else if the level price is too high
    } else if (should_create && ask->price() > price) {
      // Insert a slot
      insert_level_before(ask, last_ask_level(), price);
      break;
    }
  }
  if (ask == past_end) {
    return NULL;
  }
  return ask;
}

template <int SIZE> 
void
Depth<SIZE>::insert_level_before(DepthLevel* level, 
                                 DepthLevel* last_level, 
                                 Price price)
{
  // Back from end
  DepthLevel* current_level = last_level - 1;
  // Increment only once
  ++last_change_;
  // Last level to process is one passed in
  while (current_level >= level) {
    // Copy level to level one lower
    *(current_level + 1) = *current_level;
    // If the level being copied is valid
    if (current_level->price() != INVALID_LEVEL_PRICE) {
      // Update change Id
      (current_level + 1)->last_change(last_change_);
    }
    // Move back one
    --current_level;
   }
   level->init(price);
}

template <int SIZE> 
void
Depth<SIZE>::erase_level(DepthLevel* level, DepthLevel* last_level)
{
  // Increment once
  ++last_change_;
  DepthLevel* current_level = level;
  // Level to end
  while (current_level < last_level) {
    // If this is the first level, or the level to be overwritten is valid
    // (must force first level, when called already should be invalidated)
    if ((current_level->price() != INVALID_LEVEL_PRICE) ||
        (current_level == level)) {
      // Copy to current level from one lower
      *current_level = *(current_level + 1);
      // Mark the current level as updated
      current_level->last_change(last_change_);
    }
    // Move forward one
    ++current_level;
  }

  if (last_level->price() != INVALID_LEVEL_PRICE) {
    // Last level is blank
    last_level->init(INVALID_LEVEL_PRICE);
    last_level->last_change(last_change_);
  }
}

} }

#endif
