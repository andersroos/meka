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

   // Before even 60 minutes in the future.
   BOOST_CHECK(not eq.before(0, 0, -4 * MINUTE));            
}

