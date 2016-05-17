#include <string>

#include <boost/test/unit_test.hpp>

#include "mock.hpp"
#include "lib/util.hpp"
#include "lib/event.hpp"

using namespace std;

constexpr uint32_t MAX_TIMESTAMP = 0xffffffff; // 4.23e9 us or 71.58 minutes
constexpr uint32_t MINUTE = 6e7;

BOOST_AUTO_TEST_CASE(test_assumption_that_timestamp_32_bit_i_think_the_code_relies_on_it)
{
   timestamp_t t = 0;
   t = t - 1;
   BOOST_CHECK_EQUAL(MAX_TIMESTAMP, t);
}
   
BOOST_AUTO_TEST_CASE(test_before_operator)
{
   event_queue eq;
   // Regular before.
   BOOST_CHECK(eq.before(0, 0, 1));

   // Before if right at now.
   BOOST_CHECK(eq.before(MAX_TIMESTAMP, MAX_TIMESTAMP, 0));

   // Before even if 60 minutes in the future.
   BOOST_CHECK(eq.before(0, 0, 60 * MINUTE));
   
   // No longer before because 71 minutes is too close to now.
   BOOST_CHECK(not eq.before(0, 0, 71 * MINUTE));            

   // Now is not before 4 minutes ago...
   BOOST_CHECK(not eq.before(0, 0, -4 * MINUTE));            

   // ...but now is before 6 minutes ago.
   BOOST_CHECK(eq.before(0, 0, -6 * MINUTE));

   // 58 minutes is before 30 if now is 60.
   BOOST_CHECK(eq.before(60 * MINUTE, 58 * MINUTE, 30 * MINUTE));   
}

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

