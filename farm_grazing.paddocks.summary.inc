<?php

/**
 * Paddock summary page callback
 */
function farm_grazing_plan_summary_page($plan_obj) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // get the plan id from the plan object
  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  $num_rotations = $wrapper->field_expected_rotations->value();
  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();
  $farm_grazing_factors = $wrapper->field_farm_grazing_factors->value();
  $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
  $days_of_drought_reserve = $wrapper->field_days_of_drought_reserve->value();

  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();
  $plan_days = floor(($end_ts - $start_ts) / 3600 / 24);

  $expected_days_non_growth = $plan_days - $days_of_drought_reserve + $days_bulk_feeding;

  if ($grazing_growing_season) {

    // Set the page title
    drupal_set_title(t('Paddock Summary and Grazing Periods for Growing Season Plan'));

    $table = array(
      'header' => array(
        t('Paddock'),
        t('Area'),
        t('Quality'),
        t('Act. Min. Grazing Days'),
        t('Act. Max. Grazing Days'),
      ),
      'rows' => array(),
      'empty' => t('No paddocks found.'),
    );

    $chk_min_recovery = -1;
    $chk_max_recovery = -1;
    $paddocks = getGrazingDaysForPlanId($plan, $chk_min_recovery, $chk_max_recovery, false);

    $table['rows'] = $paddocks;

    // markup
    $form['text'] = array(
      '#markup' => t('
        <p>Check to see that recovery periods are adequate in paddocks with longer grazing periods.</p>
        <h1>Actual Min/Max Grazing Periods</h1>
      '),
    );

    $form['table'] = array(
      '#markup' => theme('table', $table),
    );

    $form['summary']['check_min'] = array(
      '#prefix' => '<p>',
      '#markup' => t('Check Min. Recovery Period: ') . round($chk_min_recovery),
      '#suffix' => '</p>',
    );

    $form['summary']['check_max'] = array(
      '#prefix' => '<p>',
      '#markup' => t('Check Max. Recovery Period: ') . round($chk_max_recovery),
      '#suffix' => '</p>',
    );
  }
  else {

    // Set the page title
    drupal_set_title(t('Paddock Summary and Grazing Periods for Non-Growing Season Plan'));

    $table = array(
      'header' => array(
        t('Paddock'),
        t('Area'),
        t('ADA/H'),
        t('Available ADs'),
        t('Max. Grazing Days'),
        t('Planned Harvest per Rotation (ADA)'),
        t('Remaining ADA after First Rotation'),
        t('Planned Total Harvest (ADA)'),
        t('Forage Excess/Deficit (ADA) - Planned Drought Reserve'),
        t('Available Drought ADs'),
        t('Max. Drought Grazing Days'),
      ),
      'rows' => array(),
      'empty' => t('No paddocks found.'),
    );

    $paddocks = getNonGrowingSummaryForPlanId($plan);

    $table['rows'] = $paddocks;

    $herd_size = getHerdSizeInSau($plan);
    $num_paddocks = getPaddockCountForPlan($plan);
    $tot_ads = empty($paddocks) ? 0 : $paddocks[$num_paddocks][3];
    $avg_ads = $tot_ads / $num_paddocks;
    $tot_grazing_days = $plan_days;
    $est_carrying_capacity = $tot_ads / $tot_grazing_days;
    $recovery_period = $tot_grazing_days / $num_rotations;
    $grazing_period = $recovery_period / $num_paddocks;
    $drought_reserve_grazing_period = $days_of_drought_reserve / $num_paddocks;

    if ($herd_size > $est_carrying_capacity) {
      drupal_set_message(t('Herd size is TOO BIG!'), 'error');
    }

    // markup
    $form['text'] = array(
      '#markup' => t('Paddock Summary for Non-Growing Season Plan'),
    );

    $form['table'] = array(
      '#markup' => theme('table', $table),
    );

    $form['summary']['avg_ads'] = array(
      '#prefix' => '<p>' . t('Average ADs: '),
      '#markup' => round($avg_ads, 1),
      '#suffix' => '</p>',
    );

    $form['summary']['tot_ads'] = array(
      '#prefix' => '<p>' . t('Total AD Available: '),
      '#markup' => $tot_ads,
      '#suffix' => '</p>',
    );

    $form['summary']['tot_grazing_days'] = array(
      '#prefix' => '<p>' . t('Total Grazing Days Required: '),
      '#markup' => $tot_grazing_days,
      '#suffix' => '</p>',
    );

    $form['summary']['herd_size'] = array(
      '#prefix' => '<p>' . t('Herd Size (SAU): '),
      '#markup' => $herd_size,
      '#suffix' => '</p>',
    );

    $form['summary']['est_carrying_capacity'] = array(
      '#prefix' => '<p>' . t('Estimated Carrying Capacity: '),
      '#markup' => round($est_carrying_capacity, 1),
      '#suffix' => '</p>',
    );

    $form['summary']['num_rotations'] = array(
      '#prefix' => '<p>' . t('Number of Rotations: '),
      '#markup' => $num_rotations,
      '#suffix' => '</p>',
    );

    $form['summary']['recovery_period'] = array(
      '#prefix' => '<p>' . t('Recovery Period: '),
      '#markup' => round($recovery_period, 1),
      '#suffix' => '</p>',
    );

    $form['summary']['num_paddocks'] = array(
      '#prefix' => '<p>' . t('Number of Paddocks: '),
      '#markup' => $num_paddocks,
      '#suffix' => '</p>',
    );

    $form['summary']['grazing_period'] = array(
      '#prefix' => '<p>' . t('Grazing Period: '),
      '#markup' => round($grazing_period, 1),
      '#suffix' => '</p>',
    );

    $form['summary']['drought_reserve_grazing_period'] = array(
      '#prefix' => '<p>' . t('Drought Reserve Grazing Period (in days): '),
      '#markup' => round($drought_reserve_grazing_period, 1),
      '#suffix' => '</p>',
    );

  }

  return $form;
}
