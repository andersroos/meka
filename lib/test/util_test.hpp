#include <string>

#include <boost/test/unit_test.hpp>

#include "mock.hpp"
#include "lib/util.hpp"

using namespace std;

BOOST_AUTO_TEST_CASE(test_assumption_that_timestamp_t_is_32_bit_as_the_code_relies_on_it)
{
   timestamp_t t = 0;
   t = t - 1;
   BOOST_CHECK_EQUAL(MAX_TIMESTAMP, t);
}
   
BOOST_AUTO_TEST_CASE(test_before)
{
   // Regular before.
   BOOST_CHECK(before(0, 0, 1));

   // Before if right at now.
   BOOST_CHECK(before(MAX_TIMESTAMP, MAX_TIMESTAMP, 0));

   // Before even if 60 minutes in the future.
   BOOST_CHECK(before(0, 0, 60 * MINUTE));
   
   // No longer before because 71 minutes is too close to now.
   BOOST_CHECK(not before(0, 0, 71 * MINUTE));            

   // Now is not before 4 minutes ago...
   BOOST_CHECK(not before(0, 0, -4 * MINUTE));            

   // ...but now is before 6 minutes ago.
   BOOST_CHECK(before(0, 0, -6 * MINUTE));

   // 58 minutes is before 30 if now is 60.
   BOOST_CHECK(before(60 * MINUTE, 58 * MINUTE, 30 * MINUTE));   
}
