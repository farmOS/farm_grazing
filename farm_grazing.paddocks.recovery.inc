<?php

/**
 * Paddock recovery page callback.
 */
function farm_grazing_plan_recovery_form($form, &$form_state, $plan_obj) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // Set the page title.
  drupal_set_title(t('Determine recovery periods'));

  // get the plan id from the plan object
  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);
  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  $header = array(
    t('Month'),
    t('Min. Recovery Days'),
    t('Max. Recovery Days'),
  );

  // get the start date month and year
  $start_date = new DateTime();
  $start_date->setTimestamp($start_ts);

  // get the end date month and year
  $end_date = new DateTime();
  $end_date->setTimestamp($end_ts);

  // get the DateInterval between them
  $diff = $start_date->diff($end_date);

  // get the number of paddocks in the plan
  $num_paddocks = getPaddockCountForPlan($plan);

  $form['#tree'] = TRUE;

  $disabled = FALSE;
  // non-growing season here.
  if (!$grazing_growing_season) {

    $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
    $days_of_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
    $start_ts = $wrapper->field_farm_date_range->value->value();
    $end_ts = $wrapper->field_farm_date_range->value2->value();
    $expected_days_non_growth = floor(($end_ts - $start_ts) / 3600 / 24);
    $expected_rotations = $wrapper->field_expected_rotations->value();

    $recovery_period = ($expected_days_non_growth + $days_of_drought_reserve
      - $days_bulk_feeding) / $expected_rotations;

    $disabled = TRUE;

    $form['text'] = array(
      '#markup' => t('<p>Recovery period is computed for non-growing season plans.</p>'),
    );

  }
  else {

    $form['text'] = array(
      '#markup' => t('<p>Enter your minimum and maximum recovery periods (in days) for each planned month of the growing season plan.</p>'),
    );

  }

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  if ($grazing_growing_season) {
    $minmax = getPlanRecovery($plan);
  }
  else {
    $minmax = array();
  }

  $form['months'] = array(
    '#theme' => 'farm_grazing_paddocks_recovery',
    '#header' => $header,
  );

  // generate array of months between start and end
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
  $herd_cnt = getHerdCountForPlan($plan);
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
 * getPlanRecovery($plan)
 */
function getPlanRecovery($plan) {
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

  foreach ($form_state['input']['months'] as $id => $val) {
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
