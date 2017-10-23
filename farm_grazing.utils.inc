<?php

// -------------- utility functions ----------------

/**
 * Utility function to fetch things from farmOS
 *
 * getHerdCountForPlan(plan_id)
 *
 */
function getHerdCountForPlan($plan_id) {
  // return the equiv. to SELECT count(herd_id) FROM {farm_grazing_herds}
  // WHERE planid=$plan
  $query = db_select('farm_grazing_herds', 'h')
    ->fields('h', array('herd_id'))
    ->condition('h.plan_id', $plan_id);

  return $query->countQuery()->execute()->fetchField();
}

/**
 * getPaddockCountForPlan($plan)
 */
function getPaddockCountForPlan($plan_id) {
  $query = db_select('farm_grazing_plan_paddock', 'a')
    ->fields('a', array('paddock_id'))
    ->condition('a.plan_id', $plan_id);

  return $query->countQuery()->execute()->fetchField();
}

/**
 * Get a list of all paddock_ids from farm_grazing_plan_paddock
 *
 * @param $plan_id
 *   The plan ID.
 *
 * @return array
 *   An array of paddock IDs.
 */
function get_selected_paddock_for_plan($plan_id) {
  // query table of paddocks for this plan
  // and build an array of those that are currently selected
  $paddocks = array();
  $results = db_query('select paddock_id from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan_id));
  foreach($results as $record) {
    $paddocks[] = $record->paddock_id;
  }
  return $paddocks;
}

/**
 * Get a list of all paddock_ids and quality from farm_grazing_plan_paddock
 */
function get_selected_paddock_quality_for_plan($plan) {
  // query table of paddocks for this plan 
  // and build an array of those that are currently selected
  $selected = array();
  $results = db_query('select paddock_id, quality from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan));
  foreach($results as $record) {
    $selected[$record->paddock_id] = $record->quality;
  }
  return $selected;
}

/**
 * TODO document this
 */
function getHerdEarliestArrival($herd_id) {
  $data = array();
  $summary = getHerdDataAndSummary($herd_id, $data);
  return $summary['arrival'];
}

/**
 * getPaddockGrazingDays()
 *
 * @param duration - current saved value
 * @param paddock_id int
 * @param mode int
 *  0 = use saved value or avg if saved is null
 *  1 = force return to min grazing days
 *  2 = force return to avg grazing days
 *  3 = force return to max grazing days
 *
 */
function getPaddockGrazingDays($duration, $paddock_id, $plan_id, $herd_id, &$cache, $mode = 0) {
  if ($cache == NULL) {
    // build the cache as we are going to need it
    $paddocks = getGrazingDaysForPlanId($plan_id, $d1, $d2, true);
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
      else {  // otherwise set it to the average
        if (isset($cache[$paddock_id])) {
          $days = ($cache[$paddock_id][3] + $cache[$paddock_id][4]) / 2.0;
        }
      }
      break;
    case 1:  //set the grazing days to minimum
      if (isset($cache[$paddock_id])) {
        $days = $cache[$paddock_id][3];
      }
      break;
    case 2:  // set the grazing days to average
      if (isset($cache[$paddock_id])) {
        $days = ($cache[$paddock_id][3] + $cache[$paddock_id][4]) / 2.0;
      }
      break;
    case 3:  // set the grazing days to maximum
      if (isset($cache[$paddock_id])) {
        $days = $cache[$paddock_id][4];
      }
      break;
  }

  return $days;
}

/**
 *
 */
function getGrazingDaysForPlanId($plan_id, &$chk_min_recovery, &$chk_max_recovery, $include_ids = true) {

  // get the plan id
  $plan = $plan_id;

  // get the herd count
  $herd_cnt = getHerdCountForPlan($plan);
  if ($herd_cnt == 0) {
    $herd_cnt = 1;
  }

  // get the paddock count
  $paddock_cnt = getPaddockCountForPlan($plan);

  $results = db_query('SELECT min(min_recovery) as minmin, max(max_recovery) as maxmax FROM {farm_grazing_plan_recovery} WHERE plan_id=:plan', array(':plan' => $plan));
  foreach($results as $record) {
    $minmin = $record->minmin;
    $maxmax = $record->maxmax;
  }

  // avoid divide by zero
  $divisor = round($paddock_cnt - $herd_cnt);
  if ($divisor == 0) {
    $divisor = 1;
  }

  $avg_min_graz = $minmin/$divisor;
  $avg_max_graz = $maxmax/$divisor;

  // get a list of selected paddocks (paddock_id, quality)
  $paddocks = array();
  $sum_tot_quality = 0;
  $results = db_query('select paddock_id, quality from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan));
  foreach($results as $record) {
    $term = taxonomy_term_load($record->paddock_id);
    $area = farm_area_calculate_area($record->paddock_id);
    $quality = $record->quality;
    $data = array(
      $term->name,
      round($area, 1),
      round($area*$quality, 1),
      $area*$quality*$avg_min_graz,
      $area*$quality*$avg_max_graz,
    );
    if ($include_ids) {
      $data[] = $record->paddock_id;
    }
    $paddocks[] = $data;
    $sum_tot_quality = $sum_tot_quality + $area*$quality;
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
  for ($i=0; $i<count($paddocks); $i++) {
    if ($avg_days > 0) {
      $paddocks[$i][3] = round($paddocks[$i][3]/$avg_days, 1);
      $paddocks[$i][4] = round($paddocks[$i][4]/$avg_days, 1);
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
function getFarmPlanDatesFromId($plan_id, $format=NULL) {
  $plan_obj = farm_plan_load($plan_id);
  return getFarmPlanDates($plan_obj, $format);
}

/**
 * Get Farm Plan Dates from plan object and optionally format them.
 */
function getFarmPlanDates($plan_obj, $format=NULL) {

  // get the start date
  $start_date = new DateTime();
  $start_date->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value']);

  // get the start date
  $end_date = new DateTime();
  $end_date->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value2']);

  if ($format == NULL) {
    return array($start_date, $end_date);
  }
  else {
    return array($start_date->format($format), $end_date->format($format));
  }
}


/**
 * daysAvailableByMonth($startDate1, $endDate1, $startDate2, $endDate2)
 * Compute the days overlap per month between start and end dates
 * for the given year
 *
 * @param $startDate1   string YYYY-MM-DD
 * @param $endDate1     string YYYY-MM-DD
 * @param $startDate2   string YYYY-MM-DD
 * @param $endDate2     string YYYY-MM-DD
 * @returns array of ints, number of days in each month of overlap
 */
function daysAvailableByMonth($startDate1, $endDate1, $startDate2, $endDate2) {
  if ($startDate1 > $endDate1 or $startDate2 > $endDate2) {
    return array();
  }

  $days = array();
  list($year_start, $month_start, $dom_start) = explode('-', $startDate1);
  list($year_end, $month_end, $dom_end) = explode('-', $endDate1);

  $num_months = 0;
  $delta = $year_end - $year_start;
  if ($delta > 0) {
    // add months to end of start year
    // and add months into end year
    $num_months += 12 - $month_start + 1 + $month_end;
    // add months for intervening years if any
    $num_months += 12 * ($delta - 1);
  }
  else {
    // add the number of months between start and end in same year
    $num_months += $month_end - $month_start + 1;
  }

  $year = $year_start;
  $mon = $month_start;
  for ($i=0; $i<$num_months+1; $i++) {
    $month_start = $year . '-' . sprintf('%02d', $mon) . '-01';
    if ($mon < 12) {
      $month_end = $year . '-' . sprintf('%02d', $mon+1) . '-01';
    }
    else {
      $month_end = ($year+1) . '-01-01';
    }

    // increment and adjust month, year
    $mon++;
    if ($mon > 12) {
      $mon = 0;
      $year++;
    }

    $days[] = overlapInDays($month_start, $month_end, $startDate2, $endDate2);
  }

  return $days;
}

/**
 * compute the overlapInDays($startDate1, $endDate1, $startDate2, $endDate2)
 * What is the overlap, in whole days, of two time periods?
 *
 * @param $startDate1   string
 * @param $endDate1     string
 * @param $startDate2   string
 * @param $endDate2     string
 * @returns int     Overlap in days
 */
function overlapInDays($startDate1, $endDate1, $startDate2, $endDate2) {
  $overlap = overlapInMinutes($startDate1, $endDate1, $startDate2, $endDate2);
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
function overlapInMinutes($startDate1, $endDate1, $startDate2, $endDate2) {
  // Figure out which is the later start time
  $lastStart = $startDate1 >= $startDate2 ? $startDate1 : $startDate2;
  // Convert that to an integer
  $lastStart = strtotime($lastStart);

  // Figure out which is the earlier end time
  $firstEnd = $endDate1 <= $endDate2 ? $endDate1 : $endDate2;
  // Convert that to an integer
  $firstEnd = strtotime($firstEnd);

  // Subtract the two, divide by 60 to convert seconds to minutes, and round down
  $overlap = floor( ($firstEnd - $lastStart) / 60 );

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
function get_days_between_dates($date1, $date2) {
  if ($date1 > $date2) {
    return 0;
  }
  // convert to an integer, seconds from epoch
  $date1 = strtotime($date1);
  $date2 = strtotime($date2);

  $minutes = floor( ($date2 - $date1) / 60 );
  $days = floor( $minutes / 60 / 24 );

  return $days;
}


/**
 * getAnimalTypeName($type)
 *
 * Return name field from {farm_grazing_animal_types} table
 */
function getAnimalTypeName($type) {
  return db_query('SELECT name FROM {farm_grazing_animal_types} WHERE type_id=:tid', array(':tid' => $type))->fetchField();
}

/**
 * getAnimalTypeDmiFactor($type)
 *
 * Return dmi_factor field from {farm_grazing_animal_types} table
 */
function getAnimalTypeDmiFactor($type) {
  return db_query('SELECT dmi_factor FROM {farm_grazing_animal_types} WHERE type_id=:tid', array(':tid' => $type))->fetchField();
}


/**
 * get_grazing_group_detailed_info($asset_id, &$data)
 *
 * Recursively walk a nested group structure extracting records
 * for each leaf node.
 *
 * Usage:
 *  $data = array();
 *  get_grazing_group_detailed_info($asset_id, $data);
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
function get_grazing_group_detailed_info($asset_id, &$data) {
  // get the asset
  $asset = farm_asset_load($asset_id);

  // ignore inactive ones
  if (!$asset->active) {
    return;
  }

  // get the asset children and process them
  $children  = farm_group_get_members($asset);
  if ($asset->type == 'group' and ! empty($children)) {
    foreach ($children as $child) {
      get_grazing_group_detailed_info($child->id, $data);
    }
  }
  else {
    // collect the attributes for a leaf node
    $type = farm_asset_property_get($asset_id, 'farm_grazing_animal_type');
    $arrival = farm_asset_property_get($asset_id, 'farm_grazing_planned_arrival');
    $departure = farm_asset_property_get($asset_id, 'farm_grazing_planned_departure');

    // get the group quantity for asset count
    $head_count = farm_inventory($asset);

    $weight = farm_grazing_asset_weight($asset);
    $dmi_factor = getAnimalTypeDmiFactor($type);

    // add the record to the results
    $data[] = array(
      'id' => $asset_id,
      'name' => $asset->name,
      'type' => $type,
      'arrival' => $arrival,
      'departure' => $departure,
      'head_count' => $head_count,
      'weight' => $weight['value'],
      'weight_units' => $weight['units'],
      'dmi_factor' => $dmi_factor,
    );
  }
}

/**
 * summarize_grazing_group_detailed_info(&$data)
 *
 * Take the results of get_grazing_group_detailed_info()
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
 *  get_grazing_group_detailed_info($asset_id, $data);
 *  $summary = summarize_grazing_group_detailed_info($data);
 */
function summarize_grazing_group_detailed_info(&$data) {
  $summary = array();
  $sum_sau = 0;
  $sum_dmi = 0;
  $head_count = 0;
  $min_arrival = false;
  $max_departure = false;

  // summarize the data
  foreach ($data as $rec) {
    $sum_sau = $sum_sau + $rec['head_count'] * $rec['weight'] / 1000.0;
    $sum_dmi = $sum_dmi + $rec['head_count'] * $rec['weight'] * $rec['dmi_factor'] / 100.0;
    $head_count = $head_count + $rec['head_count'];

    // date strings MUST be for as 'Y-m-d' ie: YYYY-MM-DD
    if ($min_arrival === false or $rec['arrival'] < $min_arrival) {
      $min_arrival = $rec['arrival'];
    }
    if ($max_departure === false or $rec['departure'] > $max_departure) {
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
 * getHerdDataAndSummary($herd_id, &$data)
 */
function getHerdDataAndSummary($herd_id, &$data) {
  get_grazing_group_detailed_info($herd_id, $data);
  return summarize_grazing_group_detailed_info($data);
}

/**
 * TODO Remove dummy function farm_group_get_members
 */
function farm_group_get_members($obj) {
  return array();
}
