#include <unity.h>

void setUp(void) {}
void tearDown(void) {}

void test_motor_moving_up(void);
void test_motor_moving_down(void);
void test_same_button_toggle_up(void);
void test_same_button_toggle_down(void);
void test_toggle_style_up_down(void);
void test_toggle_style_down_up(void);
void test_normal_stop_while_moving_up(void);
void test_stop_failure_while_moving_up(void);
void test_limit_reached_stops_and_sets_idle(void);
void test_limit_reached_stop_failure_enters_fault(void);
void test_move_up_failure_from_idle(void);
void test_move_down_failure_from_idle(void);
void test_blinds_state_fault_up(void);
void test_blinds_state_fault_down(void);
void test_blinds_state_fault_stop(void);
void test_blinds_state_fault_limit_reached(void);

void test_up_stall_backs_off_and_enters_recovery(void);
void test_down_stall_backs_off_clamped_to_max_and_enters_recovery(void);
void test_stall_recovery_limit_reached_retries_original_up_target(void);
void test_stall_recovery_limit_reached_retries_original_down_target(void);
void test_stall_recovery_retry_stop_failure_enters_fault(void);
void test_three_stall_recoveries_are_allowed_then_fourth_stall_faults(void);
void test_target_reached_resets_stall_recovery_count(void);
void test_stop_resets_stall_recovery_count(void);
void test_stall_stop_failure_enters_fault(void);
void test_stall_recovery_move_failure_enters_fault(void);

void test_home_calibration_stall_recovers_and_continues_calibrating_home(void);
void test_max_calibration_stall_before_threshold_recovers_and_continues_calibrating_max(void);
void test_calibration_stall_fourth_recovery_enters_fault(void);
void test_calibrate_move_failure_enters_fault(void);
void test_homing_reached_move_to_max_failure_enters_fault(void);
void test_calibration_returns_to_step_computed_at_homing(void);
void test_calibration_return_target_clamps_to_calibrated_max(void);
void test_calibration_return_move_failure_enters_fault(void);
void test_up_during_home_calibration_stops_and_enters_fault(void);
void test_up_during_max_calibration_stops_and_enters_fault(void);
void test_down_during_home_calibration_stops_and_enters_fault(void);
void test_down_during_max_calibration_stops_and_enters_fault(void);

void test_percent_target_moves_up_with_rounding(void);
void test_percent_target_moves_down(void);
void test_percent_endpoint_targets_use_zero_and_max(void);
void test_percent_target_rejected_before_calibrated_max(void);
void test_percent_command_retargets_while_moving(void);
void test_percent_retarget_stop_failure_enters_fault(void);
void test_percent_during_calibration_stops_and_enters_fault(void);

extern "C" void app_main(void) {
    UNITY_BEGIN();

    RUN_TEST(test_motor_moving_up);
    RUN_TEST(test_motor_moving_down);
    RUN_TEST(test_same_button_toggle_up);
    RUN_TEST(test_same_button_toggle_down);
    RUN_TEST(test_toggle_style_up_down);
    RUN_TEST(test_toggle_style_down_up);
    RUN_TEST(test_normal_stop_while_moving_up);
    RUN_TEST(test_stop_failure_while_moving_up);
    RUN_TEST(test_limit_reached_stops_and_sets_idle);
    RUN_TEST(test_limit_reached_stop_failure_enters_fault);
    RUN_TEST(test_move_up_failure_from_idle);
    RUN_TEST(test_move_down_failure_from_idle);
    RUN_TEST(test_blinds_state_fault_up);
    RUN_TEST(test_blinds_state_fault_down);
    RUN_TEST(test_blinds_state_fault_stop);
    RUN_TEST(test_blinds_state_fault_limit_reached);

    RUN_TEST(test_up_stall_backs_off_and_enters_recovery);
    RUN_TEST(test_down_stall_backs_off_clamped_to_max_and_enters_recovery);
    RUN_TEST(test_stall_recovery_limit_reached_retries_original_up_target);
    RUN_TEST(test_stall_recovery_limit_reached_retries_original_down_target);
    RUN_TEST(test_stall_recovery_retry_stop_failure_enters_fault);
    RUN_TEST(test_three_stall_recoveries_are_allowed_then_fourth_stall_faults);
    RUN_TEST(test_target_reached_resets_stall_recovery_count);
    RUN_TEST(test_stop_resets_stall_recovery_count);
    RUN_TEST(test_stall_stop_failure_enters_fault);
    RUN_TEST(test_stall_recovery_move_failure_enters_fault);

    RUN_TEST(test_home_calibration_stall_recovers_and_continues_calibrating_home);
    RUN_TEST(test_max_calibration_stall_before_threshold_recovers_and_continues_calibrating_max);
    RUN_TEST(test_calibration_stall_fourth_recovery_enters_fault);
    RUN_TEST(test_calibrate_move_failure_enters_fault);
    RUN_TEST(test_homing_reached_move_to_max_failure_enters_fault);
    RUN_TEST(test_calibration_returns_to_step_computed_at_homing);
    RUN_TEST(test_calibration_return_target_clamps_to_calibrated_max);
    RUN_TEST(test_calibration_return_move_failure_enters_fault);
    RUN_TEST(test_up_during_home_calibration_stops_and_enters_fault);
    RUN_TEST(test_up_during_max_calibration_stops_and_enters_fault);
    RUN_TEST(test_down_during_home_calibration_stops_and_enters_fault);
    RUN_TEST(test_down_during_max_calibration_stops_and_enters_fault);

    RUN_TEST(test_percent_target_moves_up_with_rounding);
    RUN_TEST(test_percent_target_moves_down);
    RUN_TEST(test_percent_endpoint_targets_use_zero_and_max);
    RUN_TEST(test_percent_target_rejected_before_calibrated_max);
    RUN_TEST(test_percent_command_retargets_while_moving);
    RUN_TEST(test_percent_retarget_stop_failure_enters_fault);
    RUN_TEST(test_percent_during_calibration_stops_and_enters_fault);

    UNITY_END();
}
