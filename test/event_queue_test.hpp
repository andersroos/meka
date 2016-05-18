#include <string>

#include <boost/test/unit_test.hpp>

#include "mock.hpp"
#include "lib/util.hpp"
#include "lib/event_queue.hpp"

using namespace std;

uint32_t result = 0;

void add_one_once(event_queue& eq, const timestamp_t& when) {
   result++;
};

void add_until_10(event_queue& eq, const timestamp_t& when) {
   result++;
   if (result < 10) {
      eq.enqueue(add_until_10, now_us() - MINUTE);
   }
};

BOOST_AUTO_TEST_CASE(test_queuing_works)
{
   result = 0;
   event_queue eq;
   eq.enqueue(add_one_once, now_us());
   eq.run();
   BOOST_CHECK_EQUAL(1, result);            
}

BOOST_AUTO_TEST_CASE(test_queueing_until_max_size_works)
{
   result = 0;
   event_queue eq;
   for (uint32_t i = 0; i < EVENTS_SIZE; ++i) {
      eq.enqueue(add_one_once, now_us());
   }
   eq.run();
   BOOST_CHECK_EQUAL(EVENTS_SIZE, result);            
}

BOOST_AUTO_TEST_CASE(test_adding_while_at_max_size_works)
{
   result = 0;
   event_queue eq;
   for (uint32_t i = 0; i < EVENTS_SIZE - 1; ++i) {
      eq.enqueue(add_one_once, now_us());
   }
   eq.enqueue(add_until_10, now_us() - MINUTE);
   eq.run();
   BOOST_CHECK_EQUAL(EVENTS_SIZE - 1 + 10, result);
}

