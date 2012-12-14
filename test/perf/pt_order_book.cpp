#include "impl/simple_order_book.h"

#include <iostream>
#include <stdexcept>
#include <stdlib.h>

using namespace liquibook;

template <class TypedOrderBook, class TypedOrder>
int run_test(TypedOrderBook& order_book, TypedOrder** orders, clock_t end) {
  int count = 0;
  TypedOrder** pp_order = orders;
  do {
    order_book.add(*pp_order);
// TODO optionally, provide listeners
    order_book.perform_callbacks();
    ++pp_order;
    if (*pp_order == NULL) {
      return -1;
    }
    ++count;
  } while (clock() < end);
  return (pp_order - orders);
}

bool build_and_run_test(int dur_sec, int num_to_try) {
  std::cout << "trying run of " << num_to_try << " orders";
  impl::SimpleOrderBook order_book;
  impl::SimpleOrder** orders = new impl::SimpleOrder*[num_to_try + 1];
  liquibook::Price price = rand() % 12 + 1896;
  liquibook::Quantity qty = ((rand() % 10) + 1) * 100;
  
  for (int i = 0; i <= num_to_try; ++i) {
    bool is_buy((i % 2) == 0);
    orders[i] = new impl::SimpleOrder(is_buy, price, qty);
  }
  orders[num_to_try] = NULL; // Final null
  
  clock_t start = clock();
  clock_t stop = start + (dur_sec * CLOCKS_PER_SEC);

  int count = run_test(order_book, orders, stop);
  delete [] orders;
  if (count > 0) {
    std::cout << " - complete!" << std::endl;
    std::cout << "Inserted " << count << " orders in " << dur_sec << " seconds"
              << ", or " << count / dur_sec << " insertions per sec"
              << std::endl;
    return true;
  } else {
    std::cout << " - not enough orders" << std::endl;
    return false;
  }

  return count;
}

int main(int argc, const char* argv[])
{
  int dur_sec = 3;
  if (argc > 1) {
    dur_sec = atoi(argv[1]);
    if (!dur_sec) { 
      dur_sec = 3;
    }
  }
  std::cout << dur_sec << " sec performance test of order book" << std::endl;
  
  srand(dur_sec);

  int num_to_try = dur_sec * 125000;
  while (true) {
    if (build_and_run_test(dur_sec, num_to_try)) {
      break;
    } else {
      num_to_try *= 2;
    }
  }
}

