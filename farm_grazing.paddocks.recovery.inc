<?php

/**
 * @file
 * Grazing paddock recovery form.
 */

/**
 * Paddock recovery page callback.
 */
function farm_grazing_plan_recovery_form($form, &$form_state, $plan_obj) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // Set the page title.
  drupal_set_title(t('Determine recovery periods'));

  // Get the plan id from the plan object.
  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  // Check to see if this is a growing season plan.
  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();

  // Load the start and end times for the plan.
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  // Build a table header.
  $header = array(
    t('Month'),
    t('Min. Recovery Days'),
    t('Max. Recovery Days'),
  );

  // Get the start date month and year.
  $start_date = new DateTime();
  $start_date->setTimestamp($start_ts);

  // Get the end date month and year.
  $end_date = new DateTime();
  $end_date->setTimestamp($end_ts);

  // Get the DateInterval between them.
  $diff = $start_date->diff($end_date);

  // Get the number of paddocks in the plan.
  $num_paddocks = farm_grazing_get_paddock_count_for_plan($plan);

  // Treat the form as a tree so that nested vales are accessible in submit.
  $form['#tree'] = TRUE;

  // Start with an assumption that the form elements will not be disabled.
  $disabled = FALSE;

  // If this is a non-growing season plan...
  if (!$grazing_growing_season) {

    // Load the necessary variables from the plan.
    $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
    $days_of_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
    $expected_rotations = $wrapper->field_expected_rotations->value();

    // Calculate the expected days of non-growth.
    $expected_days_non_growth = floor(($end_ts - $start_ts) / 3600 / 24);

    // Calculate the recovery period for all paddocks.
    $recovery_period = ($expected_days_non_growth + $days_of_drought_reserve
      - $days_bulk_feeding) / $expected_rotations;

    // Disable input fields.
    $disabled = TRUE;

    // Set the form description.
    $form['text'] = array(
      '#markup' => t('<p>Recovery period is computed for non-growing season plans.</p>'),
    );
  }

  // Or, if this is a growing season plan...
  else {

    // Set the form description.
    $form['text'] = array(
      '#markup' => t('<p>Enter your minimum and maximum recovery periods (in days) for each planned month of the growing season plan.</p>'),
    );
  }

  // Store the plan object in the form values.
  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  // If this is a growing season plan, get the plan recovery information.
  // Otherwise, initialize an empty array.
  if ($grazing_growing_season) {
    $minmax = farm_grazing_get_plan_recovery($plan);
  }
  else {
    $minmax = array();
  }

  // Create a form element for the fields, themed with
  // farm_grazing_paddocks_recovery.
  $form['months'] = array(
    '#theme' => 'farm_grazing_paddocks_recovery',
    '#header' => $header,
  );

  // Generate input fields for each month between the start and end dates.
  $num_months = $diff->y * 12 + $diff->m;
  $date = $start_date;
  $minmin = 999999999;
  $maxmax = -999999999;
  for ($i = 0; $i <= $num_months; $i++) {
    $key = $date->format('Ym');
    if (isset($minmax[$key])) {
      $min = $minmax[$key][0];
      $max = $minmax[$key][1];
    }
    else {
      if ($grazing_growing_season) {
        $min = 0;
        $max = 0;
      }
      else {
        $min = $recovery_period;
        $max = $recovery_period;
      }
    }
    if ($min > 0 and $min < $minmin) {
      $minmin = $min;
    }
    if ($max > 0 and $maxmax < $max) {
      $maxmax = $max;
    }
    $form['months'][$key]['month'] = array(
      '#markup' => $date->format('F Y'),
    );
    $form['months'][$key]['min'] = array(
      '#type' => 'textfield',
      '#title' => t('Min'),
      '#title_display' => 'invisible',
      '#default_value' => $min,
      '#size' => 30,
      '#maxlength' => 30,
      '#disabled' => $disabled,
    );
    $form['months'][$key]['max'] = array(
      '#type' => 'textfield',
      '#title' => t('Max'),
      '#title_display' => 'invisible',
      '#default_value' => $max,
      '#size' => 30,
      '#maxlength' => 30,
      '#disabled' => $disabled,
    );

    // increment the month
    $date->modify('+1 month');
  }

  // get herd count from database or 1.0
  $herd_cnt = farm_grazing_get_herd_count_for_plan($plan);
  if ($herd_cnt == 0) {
    $herd_cnt = 1;
  }

  $form['summary']['herd_cnt'] = array(
    '#prefix' => '<p>' . t('Number of herds: '),
    '#suffix' => t(' (Defaults to one herd if none are defined)') . '</p>',
    '#markup' => $herd_cnt,
  );

  $form['summary']['pad_cnt'] = array(
    '#prefix' => '<p>' . t('Number of paddocks: '),
    '#markup' => $num_paddocks,
    '#suffix' => '</p>',
  );

  if ($grazing_growing_season) {
    $min_grazing = round($minmin / ($num_paddocks - $herd_cnt), 1);
    $max_grazing = round($maxmax / ($num_paddocks - $herd_cnt), 1);
  }
  else {
    $min_grazing = round($recovery_period / $num_paddocks, 1);
    $max_grazing = $min_grazing;
  }

  $form['summary']['min_grazing'] = array(
    '#prefix' => '<p>' . t('Est. Min. Grazing Period: '),
    '#markup' => $min_grazing,
    '#suffix' => '</p>',
  );
  $form['summary']['max_grazing'] = array(
    '#prefix' => '<p>' . t('Est. Max. Grazing Period: '),
    '#markup' => $max_grazing,
    '#suffix' => '</p>',
  );

  if ($grazing_growing_season) {
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Save'),
      '#submit' => array('farm_grazing_plan_recovery_form_submit'),
      '#validate' => array('farm_grazing_plan_recovery_form_validate'),
    );
  }

  // Return markup.
  return $form;
}

/**
 * farm_grazing_get_plan_recovery($plan)
 */
function farm_grazing_get_plan_recovery($plan) {
  $minmax = array();
  $results = db_query('select month, min_recovery, max_recovery from {farm_grazing_plan_recovery} where plan_id=:plan', array(':plan' => $plan));
  foreach ($results as $record) {
    $minmax[$record->month] = array($record->min_recovery, $record->max_recovery);
  }
  return $minmax;

}

/**
 * Validate handler for farm_grazing_paddocks_recovery_form
 */
function farm_grazing_plan_recovery_form_validate($form, &$form_state) {

  foreach ($form_state['values']['months'] as $id => $val) {
    if (!is_numeric($val['min']) or $val['min'] < 0.0) {
      form_set_error('min_recovery', t('Min. Recovery Days must be numeric >= 0'));
    }
    if (!is_numeric($val['max']) or $val['max'] < 0.0) {
      form_set_error('max_recovery', t('Max. Recovery Days must be numeric >= 0'));
    }
    if ($val['min'] > $val['max']) {
      form_set_error('max_recovery', t('Max. Recovery Days must be greater than Min. Recovery Days'));
    }
  }
}

/**
 * Process the paddock recovery form on submit
 */
function farm_grazing_plan_recovery_form_submit($form, &$form_state) {

  $plan = $form_state['values']['plan'];

  foreach ($form_state['values']['months'] as $key => $val) {
    db_merge('farm_grazing_plan_recovery')
      ->key(array(
        'plan_id' => $plan,
        'month' => $key,
      ))
      ->fields(array(
        'min_recovery' => $val['min'],
        'max_recovery' => $val['max'],
      ))
      ->execute();
  }
}

/**
 * Implements hook_theme for farm_grazing_paddocks_recovery
 */
function theme_farm_grazing_paddocks_recovery(&$vars) {

  $form = $vars['form'];

  $rows = array();
  foreach (element_children($form) as $key) {
    $rows[] = array(
      render($form[$key]['month']),
      render($form[$key]['min']),
      render($form[$key]['max']),
    );
  }

  $header = $form['#header'];

  return theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'submit' => render($form['submit']),
  ));
}
