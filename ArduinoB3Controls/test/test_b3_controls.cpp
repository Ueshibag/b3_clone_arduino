#include <Arduino.h>
#include <unity.h>
#include "b3_controls.h"


/*
* Executed before every test.
*/
void setUp(void) {
    setup_ctrl_pins();
    set_controls_initial_state();
}

/*
* Executed after every test.
*/
void tearDown(void) {

}

// ===========================================================================
//                        Switches initial state tests
// ===========================================================================

void test_overdrive_switch_initial_state() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(OVERDRIVE_SWITCH));
    TEST_ASSERT_EQUAL(LOW, digitalRead(OVERDRIVE_LED));
}

void test_vibrato_lower_switch_initial_state() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_LOWER_SWITCH));
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_LOWER_LED));
}

void test_vibrato_upper_switch_initial_state() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_UPPER_SWITCH));
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_UPPER_LED));
}

/*
  Since we don't know the current position of the 6-position knob, we have
  to assume one among six is set and test the five others.
  The selected position is connected to GND while the 5 others are pulled-up.
*/
void test_vibrato_chorus_rotary_knob_initial_state() {

    if (digitalRead(V1) == LOW) {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V3));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C3));
    } else if (digitalRead(C1) == LOW) {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V3));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C3));
    } else if (digitalRead(V2) == LOW) {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V3));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C3));
    } else if (digitalRead(C2) == LOW) {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V3));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C3));
    } else if (digitalRead(V3) == LOW) {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C3));
    } else {
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C1));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(C2));
        TEST_ASSERT_EQUAL(HIGH, digitalRead(V3));
    }
}

void test_percussion_on_off_switch_initial_state() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_ON_OFF_SWITCH));
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_ON_OFF_LED));
}

void test_percussion_volume_switch_initial_state() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_VOLUME_SWITCH));
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_VOLUME_LED));
}

void test_percussion_delay_switch_initial_state() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_DELAY_SWITCH));
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_DELAY_LED));
}

void test_percussion_harmonic_switch_initial_state() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_HARM_SEL_SWITCH));
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_HARM_LED));
}

/*
  The Leslie switch is a 3-position one: LESLIE_STOP - LESLIE_SLOW, LESLIE_FAST.
  We assume the switch is set to one of them.
*/
void test_leslie_switch_initial_state() {
    int anlgMeasure = analogRead(LESLIE);
    uint8_t leslie_pos = get_leslie_position(anlgMeasure);
    uint8_t leslie_delta = 1;
    TEST_ASSERT_UINT8_WITHIN (leslie_delta, LESLIE_SLOW, leslie_pos);
}

/*
  The expression pedal position is not known when we run the tests: it is
  somewhere between full(127) and zero.
*/
void test_expression_pedal_initial_state() {
    int anlgMeasure = analogRead(EXPR_PEDAL);
    byte expr_pedal_pos = get_expr_pedal_position(anlgMeasure);
    byte pedal_delta = 64;
    byte pedal_expected = 64; // mid-range
    TEST_ASSERT_UINT8_WITHIN (pedal_delta, pedal_expected, expr_pedal_pos);
}

void test_set_overdrive() {
    set_overdrive(ON);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(OVERDRIVE_LED));

    byte expect_on[] = {0xC0, OVERDRIVE_ON};
    byte buf[64];
    Serialm.readBytes(buf, 2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect_on, buf, 2);

    set_overdrive(OFF);
    TEST_ASSERT_EQUAL(LOW, digitalRead(OVERDRIVE_LED));

    byte expect_off[] = {0xC0, OVERDRIVE_OFF};
    Serialm.readBytes(buf, 2);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expect_off, buf, 2);
}

void test_set_vibrato_upper() {
    set_vibrato_upper(ON);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(VIBRATO_UPPER_LED));

    set_vibrato_upper(OFF);
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_UPPER_LED));
}

void test_set_vibrato_lower() {
    set_vibrato_lower(ON);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(VIBRATO_LOWER_LED));

    set_vibrato_lower(OFF);
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_LOWER_LED));
}

void test_set_percussion() {
    set_percussion(ON);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_ON_OFF_LED));

    set_percussion(OFF);
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_ON_OFF_LED));
}

void test_set_percussion_volume() {
    set_percussion_volume(SOFT);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_VOLUME_LED));

    set_percussion_volume(NORMAL);
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_VOLUME_LED));
}

void test_set_percussion_delay() {
    set_percussion_delay(FAST);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_DELAY_LED));

    set_percussion_delay(SLOW);
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_DELAY_LED));
}

void test_set_percussion_harmonic() {
    set_percussion_harmonic(THIRD);
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_HARM_LED));

    set_percussion_harmonic(SECOND);
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_HARM_LED));
}

void test_on_overdrive_change() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(OVERDRIVE_LED));
    on_control_change(OVERDRIVE_SWITCH, set_overdrive);  // pressed
    on_control_change(OVERDRIVE_SWITCH, set_overdrive);  // released
    TEST_ASSERT_EQUAL(HIGH, digitalRead(OVERDRIVE_LED));
    on_control_change(OVERDRIVE_SWITCH, set_overdrive);  // pressed
    on_control_change(OVERDRIVE_SWITCH, set_overdrive);  // released
    TEST_ASSERT_EQUAL(LOW, digitalRead(OVERDRIVE_LED));
}

void test_on_vibrato_upper_change() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_UPPER_LED));
    on_control_change(VIBRATO_UPPER_SWITCH, set_vibrato_upper);  // pressed
    on_control_change(VIBRATO_UPPER_SWITCH, set_vibrato_upper);  // released
    TEST_ASSERT_EQUAL(HIGH, digitalRead(VIBRATO_UPPER_LED));
    on_control_change(VIBRATO_UPPER_SWITCH, set_vibrato_upper);  // pressed
    on_control_change(VIBRATO_UPPER_SWITCH, set_vibrato_upper);  // released
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_UPPER_LED));
}

void test_on_vibrato_lower_change() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_LOWER_LED));
    on_control_change(VIBRATO_LOWER_SWITCH, set_vibrato_lower);  // pressed
    on_control_change(VIBRATO_LOWER_SWITCH, set_vibrato_lower);  // released
    TEST_ASSERT_EQUAL(HIGH, digitalRead(VIBRATO_LOWER_LED));
    on_control_change(VIBRATO_LOWER_SWITCH, set_vibrato_lower);  // pressed
    on_control_change(VIBRATO_LOWER_SWITCH, set_vibrato_lower);  // released
    TEST_ASSERT_EQUAL(LOW, digitalRead(VIBRATO_LOWER_LED));
}

void test_on_vibrato_chorus_change(void) {
    on_vibrato_chorus_change(V1, VIBRATO_V1);
    // check program change
    on_vibrato_chorus_change(C1, VIBRATO_C1);
    // check program change
    on_vibrato_chorus_change(V2, VIBRATO_V2);
    // check program change
    on_vibrato_chorus_change(C2, VIBRATO_C2);
    // check program change
    on_vibrato_chorus_change(V3, VIBRATO_V3);
    // check program change
    on_vibrato_chorus_change(C3, VIBRATO_C3);
    // check program change
}

void test_on_percussion_change() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_ON_OFF_LED));
    on_control_change(PERC_ON_OFF_SWITCH, set_percussion);  // pressed
    on_control_change(PERC_ON_OFF_SWITCH, set_percussion);  // released
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_ON_OFF_LED));
    on_control_change(PERC_ON_OFF_SWITCH, set_percussion);  // pressed
    on_control_change(PERC_ON_OFF_SWITCH, set_percussion);  // released
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_ON_OFF_LED));
}

void test_on_percussion_volume_change() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_VOLUME_LED));
    on_control_change(PERC_VOLUME_SWITCH, set_percussion_volume);  // pressed
    on_control_change(PERC_VOLUME_SWITCH, set_percussion_volume);  // released
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_VOLUME_LED));
    on_control_change(PERC_VOLUME_SWITCH, set_percussion_volume);  // pressed
    on_control_change(PERC_VOLUME_SWITCH, set_percussion_volume);  // released
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_VOLUME_LED));
}

void test_on_percussion_delay_change() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_DELAY_LED));
    on_control_change(PERC_DELAY_SWITCH, set_percussion_delay);  // pressed
    on_control_change(PERC_DELAY_SWITCH, set_percussion_delay);  // released
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_DELAY_LED));
    on_control_change(PERC_DELAY_SWITCH, set_percussion_delay);  // pressed
    on_control_change(PERC_DELAY_SWITCH, set_percussion_delay);  // released
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_DELAY_LED));
}

void test_on_percussion_harmonic_change() {
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_HARM_LED));
    on_control_change(PERC_HARM_SEL_SWITCH, set_percussion_harmonic);  // pressed
    on_control_change(PERC_HARM_SEL_SWITCH, set_percussion_harmonic);  // released
    TEST_ASSERT_EQUAL(HIGH, digitalRead(PERC_HARM_LED));
    on_control_change(PERC_HARM_SEL_SWITCH, set_percussion_harmonic);  // pressed
    on_control_change(PERC_HARM_SEL_SWITCH, set_percussion_harmonic);  // released
    TEST_ASSERT_EQUAL(LOW, digitalRead(PERC_HARM_LED));
}

int run_unity_tests(void) {

    UNITY_BEGIN();

    RUN_TEST(test_overdrive_switch_initial_state);
    RUN_TEST(test_vibrato_lower_switch_initial_state);
    RUN_TEST(test_vibrato_upper_switch_initial_state);
    RUN_TEST(test_vibrato_chorus_rotary_knob_initial_state);

    RUN_TEST(test_percussion_on_off_switch_initial_state);
    RUN_TEST(test_percussion_volume_switch_initial_state);
    RUN_TEST(test_percussion_delay_switch_initial_state);
    RUN_TEST(test_percussion_harmonic_switch_initial_state);

    RUN_TEST(test_leslie_switch_initial_state);
    RUN_TEST(test_expression_pedal_initial_state);

    RUN_TEST(test_set_overdrive);
    RUN_TEST(test_on_overdrive_change);

    RUN_TEST(test_set_vibrato_upper);
    RUN_TEST(test_on_vibrato_upper_change);

    RUN_TEST(test_set_vibrato_lower);
    RUN_TEST(test_on_vibrato_lower_change);

    RUN_TEST(test_on_vibrato_chorus_change);

    RUN_TEST(test_set_percussion);
    RUN_TEST(test_on_percussion_change);

    RUN_TEST(test_set_percussion_volume);
    RUN_TEST(test_on_percussion_volume_change);

    RUN_TEST(test_set_percussion_delay);
    RUN_TEST(test_on_percussion_delay_change);

    RUN_TEST(test_set_percussion_harmonic);
    RUN_TEST(test_on_percussion_harmonic_change);

    return UNITY_END();
}


/*
* For Arduino framework
*/
void setup() {
  // Wait ~2 seconds before the Unity test runner
  // establishes connection with a board Serial interface
  delay(2000);

  run_unity_tests();
}

void loop() {
    // This is intentionally left empty
}
