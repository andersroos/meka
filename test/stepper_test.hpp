#include <string>

#include <boost/test/unit_test.hpp>

#include "mock.hpp"
#include "lib/util.hpp"
#include "lib/stepper.hpp"


#define S_ARGS 0, 1, 2, 3, 4, 5, 1

using namespace std;

BOOST_AUTO_TEST_CASE(test_simulation_scenario_move_1500)
{
   // s = 0.5at^2 => 750 = 0.5 * 2e4 * t ^ 2 => t = 0.27 => accel + decel -> t = 0.547722

   stepper s(S_ARGS, 700);
   s.target_speed(1e4);
   s.acceleration(2e4);
   s.on();
   s.target_pos(1500);

   uint32_t total_d = 0;
   while (true) {
      uint32_t start = now_us();
      uint32_t timestamp = s.step();
      if (s.is_stopped()) {
         break;
      }
      total_d += timestamp - start;
   }

   BOOST_CHECK(total_d < 5.7e5);
   BOOST_CHECK(5.3e5 < total_d);
}

BOOST_AUTO_TEST_CASE(test_simulation_scenario_move_backwards_1500)
{
   stepper s(S_ARGS, 700);
   s.target_speed(1e4);
   s.acceleration(2e4);
   s.on();
   s.target_pos(-1500);

   uint32_t total_d = 0;
   while (true) {
      uint32_t start = now_us();
      uint32_t timestamp = s.step();
      if (s.is_stopped()) {
         break;
      }
      total_d += timestamp - start;
   }

   BOOST_CHECK(total_d < 5.7e5);
   BOOST_CHECK(5.3e5 < total_d);
}

BOOST_AUTO_TEST_CASE(test_change_target_pos_mid_run)
{
   stepper s(S_ARGS, 1e6);  // High smooth delay to avoid micro stepping.
   s.target_speed(1e5);
   s.acceleration(2e4);
   s.on();
   s.target_pos(1e6);

   for (uint32_t p = 0; p < 200; ++p) {
      BOOST_CHECK(s.step());
   }
   BOOST_CHECK_EQUAL(200, s.pos());

   s.target_pos(0);

   // Decelerate, stop, accelerate backwards, then break = 599 steps.
   for (uint32_t p = 0; p < 599; ++p) {
      s.step();
   }
   BOOST_CHECK_EQUAL(0, s.pos());
}

BOOST_AUTO_TEST_CASE(test_expected_micro_stepping_level_is_used)
{
   stepper s(S_ARGS, 1);  // Low smooth delay to do max micro stepping.
   s.target_speed(1e5);
   s.acceleration(2e4);
   s.on();
   s.target_pos(1);

   uint32_t steps = 0;
   while (not s.is_stopped()) {
      s.step();
      steps++;
   }
   BOOST_CHECK_EQUAL(32, steps);
   BOOST_CHECK_EQUAL(1, s.pos());
   BOOST_CHECK_EQUAL(32, s.raw_pos());
   BOOST_CHECK_EQUAL(5, s.micro());
}
