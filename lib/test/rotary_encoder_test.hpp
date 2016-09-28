#include <string>

#include <boost/test/unit_test.hpp>

#include "mock.hpp"
#include "lib/rotary_encoder.hpp"

using namespace std;

BOOST_AUTO_TEST_CASE(test_ang_1024)
{
   rotary_encoder<0, 0, 1024> encoder;
   
   encoder.reset(0);
   BOOST_CHECK_EQUAL(0, encoder.ang());

   encoder.reset(-1);
   BOOST_CHECK_EQUAL(-1, encoder.ang());

   encoder.reset(51);
   BOOST_CHECK_EQUAL(51, encoder.ang(0));

   encoder.reset(1);
   BOOST_CHECK_EQUAL(1, encoder.ang(0));
   
   encoder.reset(511);
   BOOST_CHECK_EQUAL(511, encoder.ang(0));

   encoder.reset(512);
   BOOST_CHECK_EQUAL(-512, encoder.ang(0));
   
   encoder.reset(0);
   BOOST_CHECK_EQUAL(-90, encoder.ang(90));

   encoder.reset(0);
   BOOST_CHECK_EQUAL(90, encoder.ang(-90));

   encoder.reset(179);
   BOOST_CHECK_EQUAL(-1, encoder.ang(180));
}

BOOST_AUTO_TEST_CASE(test_ang_200)
{
   rotary_encoder<0, 0, 200> encoder;
   
   encoder.reset(0);
   BOOST_CHECK_EQUAL(0, encoder.ang());

   encoder.reset(199);
   BOOST_CHECK_EQUAL(-1, encoder.ang());

   encoder.reset(99);
   BOOST_CHECK_EQUAL(99, encoder.ang());

   encoder.reset(100);
   BOOST_CHECK_EQUAL(-100, encoder.ang());
   
   encoder.reset(99);
   BOOST_CHECK_EQUAL(99, encoder.ang());
}
