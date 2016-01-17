#include <string>

#include <boost/test/unit_test.hpp>

#include "mock.hpp"
#include "lib/stepper.hpp"

using namespace std;

BOOST_AUTO_TEST_CASE(a_test)
{
   stepper s(0, 1, 2, 3, 4, 5, 1, 900);
}
