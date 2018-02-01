<?php

/**
 * @file
 * Grazing utility functions.
 */

/**
 * Utility function to fetch things from farmOS
 *
 * farm_grazing_get_herd_count_for_plan(plan_id)
 *
 */
function farm_grazing_get_herd_count_for_plan($plan_id) {
  // return the equiv. to SELECT count(herd_id) FROM {farm_grazing_herds}
  // WHERE planid=$plan
  $query = db_select('farm_grazing_herds', 'h')
    ->fields('h', array('herd_id'))
    ->condition('h.plan_id', $plan_id);

  return $query->countQuery()->execute()->fetchField();
}

/**
 * farm_grazing_get_paddock_count_for_plan($plan)
 */
function farm_grazing_get_paddock_count_for_plan($plan_id) {
  $query = db_select('farm_grazing_plan_paddock', 'a')
    ->fields('a', array('paddock_id'))
    ->condition('a.plan_id', $plan_id);

  return $query->countQuery()->execute()->fetchField();
}

/**
 * farm_grazing_get_selected_paddock_for_plan($plan_id)
 *
 * Get a list of all paddock_ids from farm_grazing_plan_paddock
 *
 * @param $plan_id
 *   The plan ID.
 *
 * @return array
 *   An array of paddock IDs.
 */
function farm_grazing_get_selected_paddock_for_plan($plan_id) {
  // query table of paddocks for this plan
  // and build an array of those that are currently selected
  $paddocks = array();
  $results = db_query('select paddock_id from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan_id));
  foreach ($results as $record) {
    $paddocks[] = $record->paddock_id;
  }
  return $paddocks;
}

/**
 * farm_grazing_get_selected_paddock_quality_for_plan($plan)
 *
 * Get a list of all paddock_ids and quality from farm_grazing_plan_paddock
 */
function farm_grazing_get_selected_paddock_quality_for_plan($plan) {
  // query table of paddocks for this plan
  // and build an array of those that are currently selected
  $selected = array();
  $results = db_query('select paddock_id, quality from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan));
  foreach ($results as $record) {
    $selected[$record->paddock_id] = $record->quality;
  }
  return $selected;
}

/**
 * farm_grazing_get_herd_earliest_arrival($herd_id, $plan_id)
 *
 * Recursive walk through herd and find the earliest arrival date for any
 * part of the herd.
 *
 * @param $herd_id  int
 * @returns string YYYY-MM-DD
 */
function farm_grazing_get_herd_earliest_arrival($herd_id, $plan_id) {
  $data = array();
  $summary = farm_grazing_get_herd_data_and_summary($herd_id, $plan_id, $data);
  return $summary['arrival'];
}

/**
 * farm_grazing_get_paddock_grazing_days()
 *
 * @param duration - current saved value
 * @param paddock_id int
 * @param mode int
 *   0 = use saved value or avg if saved is null
 *   1 = force return to min grazing days
 *   2 = force return to avg grazing days
 *   3 = force return to max grazing days
 */
function farm_grazing_get_paddock_grazing_days($duration, $paddock_id, $plan_id, $herd_id, &$cache, $mode = 0) {
  if ($cache == NULL) {
    // build the cache as we are going to need it
    $paddocks = farm_grazing_get_grazing_days_for_plan_id($plan_id, $d1, $d2, TRUE);
    $cache = array();
    foreach ($paddocks as $p) {
      $cache[$p[5]] = $p;
    }
  }

  $days = 0;
  switch ($mode) {
    case 0:
      // use the duration if set
      if (isset($duration)) {
        $days = $duration;
      }
      // Otherwise set it to the average.
      else {
        if (isset($cache[$paddock_id])) {
          $days = ($cache[$paddock_id][3] + $cache[$paddock_id][4]) / 2.0;
        }
      }
      break;

    // Set the grazing days to minimum.
    case 1:
      if (isset($cache[$paddock_id])) {
        $days = $cache[$paddock_id][3];
      }
      break;

    // Set the grazing days to average.
    case 2:
      if (isset($cache[$paddock_id])) {
        $days = ($cache[$paddock_id][3] + $cache[$paddock_id][4]) / 2.0;
      }
      break;

    // Set the grazing days to maximum.
    case 3:
      if (isset($cache[$paddock_id])) {
        $days = $cache[$paddock_id][4];
      }
      break;
  }

  return $days;
}

/**
 * farm_grazing_get_grazing_days_for_plan_id($plan_id, &$chk_min_recovery, &$chk_max_recovery, $include_ids = true, $add_links = false)
 */
function farm_grazing_get_grazing_days_for_plan_id($plan_id, &$chk_min_recovery, &$chk_max_recovery, $include_ids = true, $add_links = false) {

  // get the plan id
  $plan = $plan_id;

  // get the herd count
  $herd_cnt = farm_grazing_get_herd_count_for_plan($plan);
  if ($herd_cnt == 0) {
    $herd_cnt = 1;
  }

  // get the paddock count
  $paddock_cnt = farm_grazing_get_paddock_count_for_plan($plan);

  $results = db_query('SELECT min(min_recovery) as minmin, max(max_recovery) as maxmax FROM {farm_grazing_plan_recovery} WHERE plan_id=:plan', array(':plan' => $plan));
  foreach ($results as $record) {
    $minmin = $record->minmin;
    $maxmax = $record->maxmax;
  }

  // avoid divide by zero
  $divisor = round($paddock_cnt - $herd_cnt);
  if ($divisor == 0) {
    $divisor = 1;
  }

  $avg_min_graz = $minmin / $divisor;
  $avg_max_graz = $maxmax / $divisor;

  // get a list of selected paddocks (paddock_id, quality)
  $paddocks = array();
  $sum_tot_quality = 0;
  $results = db_query('select paddock_id, quality from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan));
  foreach ($results as $record) {
    $term = taxonomy_term_load($record->paddock_id);
    $area = farm_area_calculate_area($record->paddock_id);
    $quality = $record->quality;
    $name = $term->name;
    if ($add_links) {
      $name = l($name, url('taxonomy/term/' . $record->paddock_id));
    }
    $data = array(
      $name,
      round($area, 1),
      round($area * $quality, 1),
      $area * $quality * $avg_min_graz,
      $area * $quality * $avg_max_graz,
    );
    if ($include_ids) {
      $data[] = $record->paddock_id;
    }
    $paddocks[] = $data;
    $sum_tot_quality = $sum_tot_quality + $area * $quality;
  }

  // get average
  if ($paddock_cnt > 0) {
    $avg_days = $sum_tot_quality / $paddock_cnt;
  }
  else {
    $avg_days = 0;
  }

  // adjust the act min/max grazing days by avg days
  // and avoid divide by zero
  $sum_min = 0;
  $sum_max = 0;
  $sum_area = 0;
  $sum_quality = 0;
  $max_min = 0;
  $max_max = 0;
  for ($i = 0; $i < count($paddocks); $i++) {
    if ($avg_days > 0) {
      $paddocks[$i][3] = round($paddocks[$i][3] / $avg_days, 1);
      $paddocks[$i][4] = round($paddocks[$i][4] / $avg_days, 1);
    }
    else {
      $paddocks[$i][3] = 0;
      $paddocks[$i][4] = 0;
    }
    $sum_area = $sum_area + $paddocks[$i][1];
    $sum_quality = $sum_quality + $paddocks[$i][2];
    $sum_min = $sum_min + $paddocks[$i][3];
    $sum_max = $sum_max + $paddocks[$i][4];
    if ($paddocks[$i][3] > $max_min) {
      $max_min = $paddocks[$i][3];
    }
    if ($paddocks[$i][4] > $max_max) {
      $max_max = $paddocks[$i][4];
    }
  }

  // append a footer row to the data
  $data = array(
    t('Sum of Paddocks'),
    round($sum_area, 1),
    round($sum_quality, 1),
    $sum_min,
    $sum_max,
  );
  if ($include_ids) {
    $data[] = -1;
  }
  $paddocks[] = $data;

  $chk_min_recovery = $sum_min - $max_min;
  $chk_max_recovery = $sum_max - $max_max;

  return $paddocks;
}

/**
 * Get Farm Plan Dates from plan id and optionally format them.
 */
function farm_grazing_get_farm_plan_dates_from_id($plan_id, $format = NULL) {
  $plan_obj = farm_plan_load($plan_id);
  return farm_grazing_get_farm_plan_dates($plan_obj, $format);
}

/**
 * Get Farm Plan Dates from plan object and optionally format them.
 */
function farm_grazing_get_farm_plan_dates($plan_obj, $format = NULL) {

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_obj->id);
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  if ($format == NULL) {
    return array($start_ts, $end_ts);
  }
  else {
    return array(date($format, $start_ts), date($format, $end_ts));
  }
}

/**
 * farm_grazing_get_num_months_between($date1, $date2)
 *
 * @param $date1    string YYYY-MM-DD
 * @param $date2    string YYYY-MM-DD
 * @returns array($start_year, $start_month, $number_of_months)
 */
function farm_grazing_get_num_months_between($date1, $date2) {
  if ($date1 > $date2) {
    return array();
  }

  list($y1, $m1, $d1) = explode('-', $date1);
  list($y2, $m2, $d2) = explode('-', $date2);

  $num_months = 0;
  $delta = $y2 - $y1;
  if ($delta > 0) {
    // add months to end of start year
    // and add months into end year
    $num_months += 12 - $m1 + 1 + $m2;
    // add months for intervening years if any
    $num_months += 12 * ($delta - 1);
  }
  else {
    // add the number of months between start and end in same year
    $num_months += $m2 - $m1 + 1;
  }

  return array($y1, $m1, $num_months);
}

/**
 * farm_grazing_days_available_by_month($startDate1, $endDate1, $startDate2, $endDate2)
 * Compute the days overlap per month between start and end dates
 * for the given year
 *
 * @param $startDate1   string YYYY-MM-DD
 * @param $endDate1     string YYYY-MM-DD
 * @param $startDate2   string YYYY-MM-DD
 * @param $endDate2     string YYYY-MM-DD
 * @returns array of ints, number of days in each month of overlap
 */
function farm_grazing_days_available_by_month($startDate1, $endDate1, $startDate2, $endDate2) {

  list($year, $mon, $num_months) = farm_grazing_get_num_months_between($startDate1, $endDate1);

  for ($i = 0; $i < $num_months + 1; $i++) {
    $month_start = $year . '-' . sprintf('%02d', $mon) . '-01';
    if ($mon < 12) {
      $month_end = $year . '-' . sprintf('%02d', $mon + 1) . '-01';
    }
    else {
      $month_end = ($year + 1) . '-01-01';
    }

    // increment and adjust month, year
    $mon++;
    if ($mon > 12) {
      $mon = 1;
      $year++;
    }

    $days[] = farm_grazing_overlap_in_days($month_start, $month_end, $startDate2, $endDate2);
  }

  return $days;
}

/**
 * farm_grazing_count_months($start_date, $end_date, &$months, $threshold=15)
 *
 * Used by Herds/History to count months grazed
 *
 * @param $start_date   int, Timestamp
 * @param $end_date     int, Timestamp
 * @param &$months      array of 12 ints
 * @param $threshold    int, number of days in a month to count the month
 */
function farm_grazing_count_months($start_date, $end_date, &$months, $threshold = 15) {

  // Convert timestamps to YYYY-MM-DD.
  $start = date('Y-m-d', $start_date);
  $end = date('Y-m-d', $end_date);

  list($year, $mon, $num) = farm_grazing_get_num_months_between($start, $end);

  for ($i = 0; $i < $num; $i++) {
    $month_start = $year . '-' . sprintf('%02d', $mon) . '-01';
    if ($mon < 12) {
      $month_end = $year . '-' . sprintf('%02d', $mon + 1) . '-01';
    }
    else {
      $month_end = ($year + 1) . '-01-01';
    }

    // increment and adjust month, year
    $mon++;
    if ($mon > 12) {
      $mon = 1;
      $year++;
    }

    // get the number of days in this month
    $days = farm_grazing_overlap_in_days($month_start, $month_end, $month_start, $month_end);
    if ($days >= $threshold) {
      $months[$mon]++;
      // Increment the total.
      $months[13]++;
    }
  }
}

/**
 * compute the farm_grazing_overlap_in_days($startDate1, $endDate1, $startDate2, $endDate2)
 * What is the overlap, in whole days, of two time periods?
 *
 * @param $startDate1   string
 * @param $endDate1     string
 * @param $startDate2   string
 * @param $endDate2     string
 * @returns int     Overlap in days
 */
function farm_grazing_overlap_in_days($startDate1, $endDate1, $startDate2, $endDate2) {
  $overlap = farm_grazing_overlap_in_minutes($startDate1, $endDate1, $startDate2, $endDate2);
  $days = round($overlap / 60 / 24);
  return $days;
}

/**
 * What is the overlap, in minutes, of two time periods?
 *
 * @param $startDate1   string
 * @param $endDate1     string
 * @param $startDate2   string
 * @param $endDate2     string
 * @returns int     Overlap in minutes
 */
function farm_grazing_overlap_in_minutes($startDate1, $endDate1, $startDate2, $endDate2) {
  // Figure out which is the later start time
  $lastStart = $startDate1 >= $startDate2 ? $startDate1 : $startDate2;
  // Convert that to an integer
  $lastStart = strtotime($lastStart);

  // Figure out which is the earlier end time
  $firstEnd = $endDate1 <= $endDate2 ? $endDate1 : $endDate2;
  // Convert that to an integer
  $firstEnd = strtotime($firstEnd);

  // Subtract the two, divide by 60 to convert seconds to minutes, and round down
  $overlap = floor(($firstEnd - $lastStart) / 60);

  // If the answer is greater than 0 use it.
  // If not, there is no overlap.
  return $overlap > 0 ? $overlap : 0;
}

/**
 * compute days between date1 and date2
 * date2 is before date1 return 0
 * @param date1 - string, YYYY-MM-DD
 * @param date2 - string, YYYY-MM-DD
 * @returns in - number of days between dates or 0
 */
function farm_grazing_get_days_between_dates($date1, $date2) {
  if ($date1 > $date2) {
    return 0;
  }
  // convert to an integer, seconds from epoch
  $date1 = strtotime($date1);
  $date2 = strtotime($date2);

  $minutes = ($date2 - $date1) / 60;
  $days = $minutes / 60 / 24;

  return $days;
}

/**
 * farm_grazing_get_animal_type_name($type)
 *
 * Return name field from {farm_grazing_animal_types} table
 */
function farm_grazing_get_animal_type_name($type) {
  return db_query('SELECT name FROM {farm_grazing_animal_types} WHERE type_id=:tid', array(':tid' => $type))->fetchField();
}

/**
 * farm_grazing_get_animal_type_dmi_factor($type)
 *
 * Return dmi_factor field from {farm_grazing_animal_types} table
 */
function farm_grazing_get_animal_type_dmi_factor($type) {
  $dmi_factor = db_query('SELECT dmi_factor FROM {farm_grazing_animal_types} WHERE type_id=:tid', array(':tid' => $type))->fetchField();
  $dmi_factor = $dmi_factor / 100;
  return $dmi_factor;
}

/**
 * farm_grazing_get_grazing_group_detailed_info($asset_id, &$data)
 *
 * Recursively walk a nested group structure extracting records
 * for each leaf node.
 *
 * Usage:
 *  $data = array();
 *  farm_grazing_get_grazing_group_detailed_info($asset_id, $data);
 *
 * Results:
 *  $data = array(
 *    array(
 *      'id' => $asset_id,
 *      'name' => $asset->name,
 *      'type' => $type,
 *      'arrival' => $arrival,
 *      'departure' => $departure,
 *      'head_count' => $head_count,
 *      'weight' => $weight['value'],
 *      'weight_units' => $weight['units'],
 *      'dmi_factor' => $dmi_factor,
 *    ),
 *    ...
 *  );
 */
function farm_grazing_get_grazing_group_detailed_info($asset_id, &$data) {
  // get the asset
  $asset = farm_asset_load($asset_id);

  // Ignore archived assets.
  // This may be redundant because farm_group_members() already filters out
  // archived assets by default.
  if (!empty($asset->archived)) {
    return;
  }

  // get the asset children and process them
  if ($asset->type == 'group') {
    $children = farm_group_members($asset);
    if (!empty($children)) {
      foreach ($children as $child) {
        farm_grazing_get_grazing_group_detailed_info($child->id, $data);
      }
    }
  }
  else {
    // collect the attributes for a leaf node
    $type = farm_asset_property_get($asset_id, 'farm_grazing_animal_type');
    $arrival = farm_asset_property_get($asset_id, 'farm_grazing_planned_arrival');
    $departure = farm_asset_property_get($asset_id, 'farm_grazing_planned_departure');

    // Get the group quantity for asset count. If it is an empty string, this
    // means that no inventory adjustments have been added, so we assume that
    // this is an individual animal and give it a head count of 1.
    $head_count = farm_inventory($asset);
    if ($head_count == '') {
      $head_count = 1;
    }

    // Get the DMI factor.
    $dmi_factor = farm_grazing_get_animal_type_dmi_factor($type);

    // Get the weight value and units, if they exist.
    $weight = farm_livestock_animal_weight($asset);
    $weight_value = !empty($weight['value']) ? $weight['value'] : '';
    $weight_units = !empty($weight['units']) ? $weight['units'] : '';

    // add the record to the results
    $data[] = array(
      'id' => $asset_id,
      'name' => $asset->name,
      'type' => $type,
      'arrival' => !empty($arrival) ? date('Y-m-d', $arrival) : '',
      'departure' => !empty($departure) ? date('Y-m-d', $departure) : '',
      'head_count' => $head_count,
      'weight' => $weight_value,
      'weight_units' => $weight_units,
      'dmi_factor' => $dmi_factor,
    );
  }
}

/**
 * farm_grazing_summarize_group_detailed_info(&$data)
 *
 * Take the results of farm_grazing_get_grazing_group_detailed_info()
 * and create a one record summary used by Herds/Manage
 *
 * Returns:
 *   array(
 *     'head_count' => $head_count,
 *     'herd_sau' => $sum_sau,
 *     'herd_dmi' => $sum_dmi,
 *     'arrival' => $min_arrival,
 *     'departure' => $max_departure)
 *
 * Usage:
 *  $data = array();
 *  farm_grazing_get_grazing_group_detailed_info($asset_id, $data);
 *  $summary = farm_grazing_summarize_group_detailed_info($data);
 */
function farm_grazing_summarize_group_detailed_info(&$data) {
  $sum_sau = 0;
  $sum_dmi = 0;
  $head_count = 0;
  $min_arrival = FALSE;
  $max_departure = FALSE;

  // summarize the data
  foreach ($data as $rec) {
    $sum_sau = $sum_sau + $rec['head_count'] * $rec['weight'] / 1000.0;
    $sum_dmi = $sum_dmi + $rec['head_count'] * $rec['weight'] * $rec['dmi_factor'];
    $head_count = $head_count + $rec['head_count'];

    // date strings MUST be for as 'Y-m-d' ie: YYYY-MM-DD
    if ($min_arrival === FALSE or $rec['arrival'] < $min_arrival) {
      $min_arrival = $rec['arrival'];
    }
    if ($max_departure === FALSE or $rec['departure'] > $max_departure) {
      $max_departure = $rec['departure'];
    }
  }

  // return result
  return array(
    'head_count' => $head_count,
    'herd_sau' => $sum_sau,
    'herd_dmi' => $sum_dmi,
    'arrival' => $min_arrival,
    'departure' => $max_departure,
  );
}

/**
 * Validate a herd in a plan.
 *
 * when a herd is attached to a grazing plan or otherwise modified
 * this function checks that all components of the herd have required
 * properties and initial observations.
 *
 * @param int $herd_id
 *   The herd (group asset) ID.
 * @param int $plan_id
 *   The farm plan ID.
 * @param array &$data
 *   An array of data to pass into farm_grazing_get_grazing_group_detailed_info().
 * @param bool $silent
 *   Whether or not to output error messages using drupal_set_message().
 *   Defaults to FALSE (errors will show).
 *
 * @return int
 *   Returns the number of errors encountered.
 */
function farm_grazing_validate_herd($herd_id, $plan_id, &$data, $silent = FALSE) {
  $errors = 0;

  farm_grazing_get_grazing_group_detailed_info($herd_id, $data);

  // Keep track of missing data.
  $missing_type = 0;
  $missing_weight = 0;

  foreach ($data as &$animal) {

    // Things to validate
    //  farm_grazing_animal_type property exists and is valid
    $type = $animal['type'];
    if (empty($type)) {
      $missing_type++;
      $errors++;
    }

    //  has weight
    $weight = $animal['weight'];
    if (empty($weight)) {
      $missing_weight++;
      $errors++;
    }

    //  TODO are all animals/groups are in some paddock

    //  TODO is asset attached to another active plan?
  }

  // Print errors (unless we should be silent).
  if (empty($silent) && !empty($errors)) {

    // General message.
    drupal_set_message('Some required data is missing from the herds/animals in this plan. Please see the errors below and add any data that may be missing. Fields that are required will be highlighted in the form below, along with any values that are calculated from those fields.', 'error');

    // Missing animal types.
    if (!empty($missing_type)) {
      drupal_set_message(t('@count animal(s) do not have an animal type.', array('@count' => $missing_weight)), 'error');
    }

    // Missing weight.
    if (!empty($missing_weight)) {
      drupal_set_message(t('@count animal(s) are missing weight measurements.', array('@count' => $missing_weight)), 'error');
    }
  }

  return $errors;
}


/**
 * farm_grazing_get_herd_data_and_summary($herd_id, $plan_id, &$data)
 */
function farm_grazing_get_herd_data_and_summary($herd_id, $plan_id, &$data) {
  farm_grazing_validate_herd($herd_id, $plan_id, $data);
  return farm_grazing_summarize_group_detailed_info($data);
}

/**
 * farm_grazing_get_herd_size_in_sau($plan_id)
 */
function farm_grazing_get_herd_size_in_sau($plan_id) {
  // get list of herds in this plan
  $query = db_query('SELECT * FROM {farm_grazing_herds} WHERE plan_id=:plan_id', array(':plan_id' => $plan_id));

  $records = $query->fetchAll();

  $tot_sau = 0;
  foreach ($records as $record) {
    if (empty($record->herd_id)) {
      continue;
    }

    $data = array();
    $summary = farm_grazing_get_herd_data_and_summary($record->herd_id, $plan_id, $data);

    $tot_sau += $summary['herd_sau'];
  }

  return $tot_sau;
}

/**
 * farm_grazing_get_non_growing_summary_for_plan_id($plan_id, $add_ids = FALSE, $add_links = FALSE)
 *
 * Compute statistics for paddocks in non-growing season plan
 */
function farm_grazing_get_non_growing_summary_for_plan_id($plan_id, $add_ids = FALSE, $add_links = FALSE) {

  // get herds size if defined
  $herd_size = farm_grazing_get_herd_size_in_sau($plan_id);

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);

  $num_rotations = $wrapper->field_expected_rotations->value();
  $num_paddocks = farm_grazing_get_paddock_count_for_plan($plan_id);
  $days_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
  $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
  $days_drought_grazing = $days_drought_reserve / $num_paddocks;

  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();
  $plan_days = floor(($end_ts - $start_ts) / 3600 / 24);

  $expected_days_non_growth = $plan_days - $days_drought_reserve + $days_bulk_feeding;

  $grazing_period = ($expected_days_non_growth + $days_drought_reserve - $days_bulk_feeding) / $num_rotations / $num_paddocks;

  // each row in $rows consists of
  // paddock, area, ada, avail_ads, max_grazing_days, planned_harvest_per_rotation, remaining_harvest_aft_first, planned_tot_harvest, forage_excess_deficit, avail_drought_ads, max_drought_grazing_days
  $rows = array();
  $sums = array(0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  $results = db_query('select paddock_id, quality from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan_id));
  foreach ($results as $record) {
    $name = taxonomy_term_load($record->paddock_id)->name;
    $area = farm_area_calculate_area($record->paddock_id);
    $ada = $record->quality;
    if ($add_links) {
      $name = l($name, url('taxonomy/term/' . $record->paddock_id));
    }
    $row = array(
      $name,                // paddock
      $area,                // 8, area (needs to be round)
      $ada,                 // 9, ada
      $area * $ada,           // 0, avail_ads (needs to be round)
      $area * $ada * $grazing_period,   // 1, max_grazing_days (needs / avg_ads)
      $herd_size * $area * $ada/$area,  // 2, planned_harvest_per_rotation (needs / avg_ads)
      $ada,                 // 3, remaining_harvest_aft_first (needs - $row[5])
      $num_rotations,       // 4, planned_tot_harvest (needs * $row[5])
      $days_bulk_feeding,   // 5, forage_excess_deficit (needs - $row[7])
      $area,                // 6, avail_drought_ads (needs * $row[8])
      $days_drought_reserve,// 7, max_drought_grazing_days (needs *$row[9]/(sums[6]/$num_paddocks
    );
    if ($add_ids) {
      $row[] = $record->paddock_id;
    }
    $rows[] = $row;

    // accumulate some sums
    // [0] available ads
    // [1] max. grazing days
    // [2] planned harvest per rotation
    for ($i = 0; $i < 2; $i++) {
      $sums[$i] += $row[$i + 3];
    }
  }

  // fix up sums and rows as needed
  $avg_ads = $sums[0] / $num_paddocks;
  // max. grazing days.
  $sums[1] = $sums[1] / $avg_ads;
  for ($i = 0; $i < $num_paddocks; $i++) {
    $rows[$i][4] = $rows[$i][4] / $avg_ads;
    $rows[$i][5] = $herd_size * $rows[$i][4] / $rows[$i][1];
    $rows[$i][6] = $rows[$i][2] - $rows[$i][5];
    $rows[$i][7] = $rows[$i][5] * $num_rotations;
    $rows[$i][8] = $rows[$i][2] - $rows[$i][7];
    $rows[$i][9] = $rows[$i][1] * $rows[$i][8];
    // accumulate more sums
    $sums[2] += $rows[$i][5]; // planned harvest per rotation
    $sums[3] += $rows[$i][6]; // remaining ada aft.first rotation
    $sums[4] += $rows[$i][7]; // planned total harvest
    $sums[5] += $rows[$i][8]; // forage excess/deficit - planned drought reserve
    $sums[6] += $rows[$i][9]; // available drought ads
  }
  $avg_avail_drought_ads = $sums[6] / $num_paddocks;
  for ($i = 0; $i < $num_paddocks; $i++) {
    $rows[$i][10] = $rows[$i][9] * $days_drought_grazing / $avg_avail_drought_ads;
    $sums[7] += $rows[$i][10];  // max. drought grazing days
    $sums[8] += $rows[$i][1];   // area
    $sums[9] += $rows[$i][2];   // ada/h
  }

  // push the totals
  $totals = array('<b>' . t('Column Sums') . '</b>', $sums[8], $sums[9]);
  for ($i = 0; $i < 8; $i++) {
    $totals[] = $sums[$i];
  }
  $rows[] = $totals;

  // push the averages
  $totals[0] = '<b>' . t('Column Average') . '</b>';
  for ($i = 1; $i < 11; $i++) {
    $totals[$i] /= $num_paddocks;
  }
  $rows[] = $totals;

  // round table values to one digit
  for ($i = 0; $i < $num_paddocks + 2; $i++) {
    for ($j = 1; $j < 11; $j++) {
      $rows[$i][$j] = round($rows[$i][$j], 1);
    }
  }

  return $rows;
}

/**
 * Load rotations for a given plan and herd.
 *
 * @param $plan_id
 *   The plan ID.
 * @param $herd_id
 *   The herd ID (group asset ID).
 *
 * @return array
 *   Returns an array of rotations loaded from the database, keyed by ID.
 */
function farm_grazing_load_rotations($plan_id, $herd_id) {
  $query = db_select('farm_grazing_rotations', 'r');
  $query->fields('r');
  $query->leftJoin('taxonomy_term_data', 't', 'r.paddock_id = t.tid');
  $query->addField('t', 'name');
  $query->condition('plan_id', $plan_id);
  $query->condition('herd_id', $herd_id);
  $query->orderBy('start_date', 'ASC');
  return $query->execute()->fetchAllAssoc('id');
}

/**
 * Recalculate the rotations for a plan and save them, along with their logs.
 *
 * @param @plan_id
 *   The plan ID.
 * @param @herd_id
 *   The herd ID (group asset ID).
 * @param $rotations
 *   Optionally, an array of rotation records to recalculate. If this is
 *   omitted, the rotations will be loaded from the database.
 * @param $reset_durations
 *   Optionally use this parameter to reset the rotation's duration (in grazing
 *   days) to one of the following options:
 *     0 = no change
 *     1 = minimum
 *     2 = average
 *     3 = maximum
 */
function farm_grazing_recalculate_rotations($plan_id, $herd_id, $rotations = array(), $reset_durations = 0) {

  // If rotations are not provided, load them from the plan.
  if (empty($rotations)) {

    // Load rotations from the database.
    $rotations = farm_grazing_load_rotations($plan_id, $herd_id);

    // If no rotations are loaded, bail.
    if (empty($rotations)) {
      return;
    }
  }

  // Load the plan's start time.
  $plan_wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
  $plan_start_time = $plan_wrapper->field_farm_date_range->value->value();

  // Make sure $reset_durations is a valid option.
  if (!in_array($reset_durations, array(0, 1, 2, 3))) {
    $reset_durations = 0;
  }

  // If we are resetting the grazing days, build grazing days data for the
  // paddocks.
  if ($reset_durations != 0) {
    $paddocks = farm_grazing_get_grazing_days_for_plan_id($plan_id, $d1, $d2, TRUE);
    $data = array();
    if (!empty($paddocks)) {
      foreach ($paddocks as $p) {
        $data[$p[5]] = $p;
      }
    }
  }

  // Iterate through the rotations, keeping track of the timestamp after each,
  // starting with the start time of the plan.
  $timestamp = $plan_start_time;
  foreach ($rotations as $rotation) {

    // Load the rotation log.
    if (!empty($rotation->log_id)) {
      $log = log_load($rotation->log_id);
    }

    // If the log is missing, stop everything and alert the user.
    if (empty($log)) {
      drupal_set_message(t('A log is missing from one or more rotations. Remove the rotation and recreate it to fix this issue.'));
      return;
    }

    // If the log is not marked "done", update the log and rotation.
    if (empty($log->done)) {

      // Update the rotation and log timestamps.
      $rotation->start_date = $timestamp;
      $log->timestamp = $timestamp;

      // Recalculate rotation duration, if desired.
      if (in_array($reset_durations, array(1, 2, 3))) {
        $cache = array();
        $rotation->duration = farm_grazing_get_paddock_grazing_days(NULL, $rotation->paddock_id, $plan_id, $rotation->herd_id, $cache, $reset_durations);
      }

      // Add a flag to the log record to indicate that we are in the process
      // of recalculating all rotations. This is used in
      // farm_grazing_plan_log_update(), which handles other cases where a
      // log's timestamp is changing. We don't want to run that code when we
      // are recalculating all the rotations, because it will cause an
      // an infinite loop.
      $log->recalculating_rotations = TRUE;

      // Update the rotation record.
      drupal_write_record('farm_grazing_rotations', $rotation, array('id'));

      // Save the log.
      log_save($log);
    }

    // Increment the timestamp by the duration in seconds.
    $timestamp += $rotation->duration * 24 * 3600;
  }
}
