#include <string>

#include <boost/test/unit_test.hpp>

#include "mock.hpp"
#include "lib/util.hpp"
#include "lib/event.hpp"

using namespace std;

BOOST_AUTO_TEST_CASE(test_assumption_that_timestamp_32_bit_i_think_the_code_relies_on_it)
{
   timestamp_t t = 0;
   t = t - 1;
   BOOST_CHECK_EQUAL(0xffffffff, t);
}
   
BOOST_AUTO_TEST_CASE(test_before_operator)
{
}

