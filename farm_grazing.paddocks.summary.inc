<?php

include_once "farm_grazing.utils.inc";

/**
 * Paddock summary page callback
 */
function farm_grazing_plan_summary_page($plan_obj) {
  
  // get the plan id from the plan object
  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();
  $farm_grazing_factors = $wrapper->field_farm_grazing_factors->value();
  $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
  $days_of_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
  $expected_days_non_growth = $wrapper->field_expected_days_non_growth->value();
  $num_rotations = $wrapper->field_expected_rotations->value();

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

    $num_paddocks = getPaddockCountForPlan($plan);
    $tot_ads = empty($paddocks) ? 0 : $paddocks[$num_paddocks][3];
    $avg_ads = $tot_ads/$num_paddocks;
    $tot_grazing_days = $expected_days_non_growth + $days_of_drought_reserve - $days_bulk_feeding;
    $est_carrying_capacity = $tot_ads / $tot_grazing_days;
    $recovery_period = $tot_grazing_days / $num_rotations;
    $grazing_period = $recovery_period / $num_paddocks;
    $drought_reserve_grazing_period = $days_of_drought_reserve /$num_paddocks;


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


/**
 *
 */
function getNonGrowingSummaryForPlanId($plan_id) {
  
  // get herds size if defined
  $herd_size = 0; // TODO

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);

  $num_rotations = $wrapper->field_expected_rotations->value();
  $num_paddocks = getPaddockCountForPlan($plan_id);
  $days_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
  $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
  $expected_days_non_growth = $wrapper->field_expected_days_non_growth->value();
  $days_drought_grazing = $days_drought_reserve / $num_paddocks;
  $grazing_period = ($expected_days_non_growth + $days_drought_reserve - $days_bulk_feeding) / $num_rotations / $num_paddocks;

  // each row in $rows consists of
  // paddock, area, ada, avail_ads, max_grazing_days, planned_harvest_per_rotation, remaining_harvest_aft_first, planned_tot_harvest, forage_excess_deficit, avail_drought_ads, max_drought_grazing_days

  $rows = array();
  $sums = array(0,0,0,0,0,0,0,0,0,0);

  $results = db_query('select paddock_id, quality from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan_id));
  foreach($results as $record) {
    $name = taxonomy_term_load($record->paddock_id)->name;
    $area = farm_area_calculate_area($record->paddock_id);
    $ada = $record->quality;
    $row = array(
      $name,                // paddock
      $area,                // area (needs to be round)
      $ada,                 // ada
      $area*$ada,           // 0, avail_ads (needs to be round)
      $area*$ada*$grazing_period,   // 1, max_grazing_days (needs / avg_ads)
      $herd_size*$area*$ada/$area,  // 2, planned_harvest_per_rotation (needs / avg_ads)
      $ada,                 // 3, remaining_harvest_aft_first (needs - $row[5])
      $num_rotations,       // 4, planned_tot_harvest (needs * $row[5])
      $days_bulk_feeding,   // 5, forage_excess_deficit (needs - $row[7])
      $area,                // 6, avail_drought_ads (needs * $row[8])
      $days_drought_reserve,// 7, max_drought_grazing_days (needs *$row[9]/(sums[6]/$num_paddocks
    );
    $rows[] = $row;

    // accumulate some sums
    for($i=0; $i<3; $i++) {
      $sums[$i] += $row[$i+3];
    }
  }

  // fix up sums and rows as needed
  $avg_ads = $sums[0] / $num_paddocks;
  $sums[1] = $sums[1] / $avg_ads;
  $sums[2] = $sums[2] / $avg_ads;
  for ($i=0; $i<$num_paddocks; $i++) {
    $rows[$i][4] = $rows[$i][4] / $avg_ads;
    $rows[$i][5] = $rows[$i][5] / $avg_ads;
    $rows[$i][6] = $days_bulk_feeding - $rows[$i][5];
    $rows[$i][7] = $rows[$i][5] * $num_rotations;
    $rows[$i][8] = $days_bulk_feeding - $rows[$i][7];
    $rows[$i][9] = $rows[$i][1] * $rows[$i][8];
    // accumulate more sums
    $sums[2] += $rows[$i][5];
    $sums[3] += $rows[$i][6];
    $sums[4] += $rows[$i][7];
    $sums[5] += $rows[$i][8];
    $sums[6] += $rows[$i][9];
  }
  $avg_avail_drought_ads = $sums[6] / $num_paddocks;
  for ($i=0; $i<$num_paddocks; $i++) {
    $rows[$i][10] = $rows[$i][9] * $days_drought_grazing / $avg_avail_drought_ads;
    $sums[7] += $rows[$i][10];
    $sums[8] += $rows[$i][1];
    $sums[9] += $rows[$i][2];
  }

  // push the totals
  $totals = array('<b>'.t('Column Sums').'</b>', $sums[8], $sums[9]);
  for ($i=0; $i<8; $i++) {
    $totals[] = $sums[$i];
  }
  $rows[] = $totals;

  // push the averages
  $totals[0] = '<b>'.t('Column Average').'</b>';
  for ($i=1; $i<11; $i++) {
    $totals[$i] /= $num_paddocks;
  }
  $rows[] = $totals;

  // round table values to one digit
  for ($i=0; $i<$num_paddocks+2; $i++) {
    for ($j=1; $j<11; $j++) {
      $rows[$i][$j] = round($rows[$i][$j], 1);
    }
  }

  return $rows;
}
