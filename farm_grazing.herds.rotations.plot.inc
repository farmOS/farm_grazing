<?php

/**
 * @file
 * Functions for plotting grazing rotations as images.
 */

/**
 * Generate an image that plots a paddock rotation visualization.
 *
 * Parameters from URL
 *
 * @param plan_obj   - drupal plan object
 * @param paddock_id - int, paddock id for this plan
 * @param start_date - string YYYY-MM-DD, start of paddock grazing
 * @param end_date   - string YYYY-MM-DD, end of paddock grazing
 *
 * @returns an image showing rotation in calendar or calendar header image
 */
function farm_grazing_plan_plot($plan_obj, $paddock_id, $start_date, $end_date) {
  define('DO_SVG', 1);

  if (DO_SVG) {
    farm_grazing_plan_plot_svg($plan_obj, $paddock_id, $start_date, $end_date);
  }
  else {
    farm_grazing_plan_plot_png($plan_obj, $paddock_id, $start_date, $end_date);
  }
}

/**
 * farm_grazing_plan_plot_svg($plan_obj, $paddock_id, $start_date, $end_date)
 *
 * @param plan_obj   - drupal plan object
 * @param paddock_id - int, paddock id for this plan
 * @param start_date - string YYYY-MM-DD, start of paddock grazing
 * @param end_date   - string YYYY-MM-DD, end of paddock grazing
 *
 * @returns an image showing rotation in calendar or calendar header image
 */
function farm_grazing_plan_plot_svg($plan_obj, $paddock_id, $start_date, $end_date) {
  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_obj->id);
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  // Set the header Content-type
  drupal_add_http_header('Content-Type', 'image/svg+xml');

  $plan = $plan_obj->id;
  $header = 0;

  // if paddock_id, start_date and end_date == 0 then generate the header image
  // If any of the arguments are empty, bail with a 404 not found.
  if (isset($plan) &&
      isset($paddock_id) && $paddock_id == 0 &&
      isset($start_date) && $start_date == 0 &&
      isset($end_date)   && $end_date == 0) {
    // generate header image
    $header = 1;
  }
  elseif (empty($plan) || empty($paddock_id) || empty($start_date) || empty($end_date)) {
    drupal_not_found();
    drupal_exit();
  }

  // set image width and height
  $width = 550;
  $height = 30;

  // get the start date month and year
  $plan_start = new DateTime();
  $plan_start->setTimestamp($start_ts);
  $plan_start = $plan_start->format('Y-m-d');

  // get the end date month and year
  $plan_end = new DateTime();
  $plan_end->setTimestamp($end_ts);
  $plan_end = $plan_end->format('Y-m-d');

  // get number of days in plan
  $plan_days = farm_grazing_get_days_between_dates($plan_start, $plan_end);

  // get number of days from start of plan to start_date
  $days_to_start = farm_grazing_get_days_between_dates($plan_start, $start_date);

  // get number of days from start of plan to end_date
  $days_to_end = farm_grazing_get_days_between_dates($plan_start, $end_date);

  // get day width in pixels and adjust image width
  $day_width = $width / ($plan_days + 1);
  $width = $plan_days * $day_width;

  if (!$header) {
    $exclusions = farm_grazing_get_paddock_exclusions($plan, $paddock_id);
    $recovery = farm_grazing_get_paddock_recovery($plan, $paddock_id, $end_date);
  }

  // Create a demo image.
  $im = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' .
    '<svg xmlns="http://www.w3.org/2000/svg"' .
    ' xmlns:xlink= "http://www.w3.org/1999/xlink"' .
    ' width="' . $width . '" height="' . $height . '"' .
    ' viewBox="0 0 ' . $width . ' ' . $height . '">';

  // svg needs css, added these to farm_grazing.herds.rotations.inc
  // svg {position:absolute; background:white;}
  // td svg, th svg {position:relative; width:100%; height:100%; margin:0 auto}

  if (!$header) {

    // add exclusion periods
    foreach ($exclusions as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 2;
      $w = ($event['eday'] - $event['sday']) * $day_width;
      $h = $height - 4;
      $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
             "style=\"fill:red;fill-opacity:1\" />";
    }

    // add recovery periods
    foreach ($recovery as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 6;
      $w = ($event['eday'] - $event['sday']) * $day_width;
      $h = $height - 12;
      $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
        "style=\"fill:orange;fill-opacity:.5\" />";
    }

    // add the paddock for grazing period
    $x1 = $days_to_start * $day_width;
    $y1 = 10;
    $w = ($days_to_end - $days_to_start) * $day_width;
    $h = $height - 20;
    $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
      "style=\"fill:green;fill-opacity:1\" />";
  }

  // draw the month boundaries
  $months = array(t('Jan'), t('Feb'), t('Mar'), t('Apr'), t('May'), t('Jun'),
    t('Jul'), t('Aug'), t('Sep'), t('Oct'), t('Nov'), t('Dec'),
  );

  $days = farm_grazing_days_available_by_month($plan_start, $plan_end, $plan_start, $plan_end);

  list($y, $m, $d) = explode('-', $plan_start);
  $sum = 0;
  foreach ($days as $num) {
    if ($num == 0) {
      continue;
    }
    $sum += $num;
    $x1 = $sum * $day_width;
    $y1 = 0;
    $x2 = $x1;
    $y2 = $height;
    $im .= "<line x1=\"$x1\" y1=\"$y1\" x2=\"$x2\" y2=\"$y2\" " .
      "style=\"stroke:black;stroke-width:1.5\" />";
    if ($header) {
      $tx = $x1 + 10;
      $ty = $height - 2;
      $txt = $months[$m % 12];
      $thgt = $height *.75;
      $im .= "<text x=\"$tx\" y=\"$ty\" fill=\"black\" font-family=\"Verdana\" font-size=\"$thgt\">$txt</text>";
    }
    $m++;
  }

  // output the image
  $im .= '</svg>';
  echo $im;

  drupal_exit();
}

/**
 * farm_grazing_plan_plot_png($plan_obj, $paddock_id, $start_date, $end_date)
 *
 * @param plan_obj   - drupal plan object
 * @param paddock_id - int, paddock id for this plan
 * @param start_date - string YYYY-MM-DD, start of paddock grazing
 * @param end_date   - string YYYY-MM-DD, end of paddock grazing
 *
 * @returns an image showing rotation in calendar or calendar header image
 */
function farm_grazing_plan_plot_png($plan_obj, $paddock_id, $start_date, $end_date) {
  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_obj->id);
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  // Set the header Content-type
  drupal_add_http_header('Content-Type', 'image/png');

  $plan = $plan_obj->id;
  $header = 0;

  // if paddock_id, start_date and end_date == 0 then generate the header image
  // If any of the arguments are empty, bail with a 404 not found.
  if (isset($plan) &&
      isset($paddock_id) && $paddock_id == 0 &&
      isset($start_date) && $start_date == 0 &&
      isset($end_date)   && $end_date == 0) {
    // generate header image
    $header = 1;
  }
  elseif (empty($plan) || empty($paddock_id) || empty($start_date) || empty($end_date)) {
    drupal_not_found();
    drupal_exit();
  }

  // set image width and height
  $width = 550;
  $height = 16;

  // get the start date month and year
  $plan_start = date('Y-m-d', $start_ts);

  // get the end date month and year
  $plan_end = date('Y-m-d', $end_ts);

  // get number of days in plan
  $plan_days = farm_grazing_get_days_between_dates($plan_start, $plan_end);

  // get number of days from start of plan to start_date
  $days_to_start = farm_grazing_get_days_between_dates($plan_start, $start_date);

  // get number of days from start of plan to end_date
  $days_to_end = farm_grazing_get_days_between_dates($plan_start, $end_date);

  // get day width in pixels and adjust image width
  $day_width = round($width / ($plan_days + 1));
  $width = $plan_days * $day_width;

  if (!$header) {
    $exclusions = farm_grazing_get_paddock_exclusions($plan, $paddock_id);
    $recovery = farm_grazing_get_paddock_recovery($plan, $paddock_id, $end_date);
  }

  // Create a demo image.
  $im = @imagecreate($width, $height)
  or die("Cannot Initialize new GD image stream");

  $white = imagecolorallocate($im, 255, 255, 255);
  imagefill($im, 0, 0, $white);

  if (!$header) {

    // add exclusion periods
    $red = imagecolorallocate($im, 255, 0, 0);
    foreach ($exclusions as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 3;
      $x2 = $event['eday'] * $day_width;
      $y2 = $height - 3;
      imagefilledrectangle($im, $x1, $y1, $x2, $y2, $red);
    }

    // add recovery periods
    $orange = imagecolorallocate($im, 255, 128, 0);
    foreach ($recovery as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 5;
      $x2 = $event['eday'] * $day_width;
      $y2 = $height - 5;
      imagefilledrectangle($im, $x1, $y1, $x2, $y2, $orange);
    }

    // add the paddock for grazing period
    $green = imagecolorallocate($im, 0, 255, 0);
    $x1 = $days_to_start * $day_width;
    $y1 = 6;
    $x2 = $days_to_end * $day_width;
    $y2 = $height - 6;
    imagefilledrectangle($im, $x1, $y1, $x2, $y2, $green);
  }

  $black = imagecolorallocate($im, 0, 0, 0);

  // draw the month boundaries
  $months = array(t('Jan'), t('Feb'), t('Mar'), t('Apr'), t('May'), t('Jun'),
    t('Jul'), t('Aug'), t('Sep'), t('Oct'), t('Nov'), t('Dec'),
  );

  $days = farm_grazing_days_available_by_month($plan_start, $plan_end, $plan_start, $plan_end);

  list($y, $m, $d) = explode('-', $plan_start);
  $sum = 0;
  foreach ($days as $num) {
    if ($num == 0) {
      continue;
    }
    $sum += $num;
    $x1 = $sum * $day_width;
    $y1 = 0;
    $x2 = $x1;
    $y2 = $height;
    imageline($im, $x1, $y1, $x2, $y2, $black);
    if ($header) {
      imagestring($im, 2, $x1 + 10, $y1 + 3, $months[$m % 12], $black);
    }
    $m++;
  }

  // output the image, 2nd param is filename, NULL sends to stdout
  imagepng($im);
  imagedestroy($im);

  drupal_exit();
}

/**
 * farm_grazing_get_paddock_exclusions($plan_id, $paddock_id)
 *
 *   $exclusions = array(
 *     array(
 *       'id' => paddock_id,
 *       'sday' => (exc_start - plan_start) as days,
 *       'eday' => (exc_end   - plan_start) as days,
 *       'type' => exclusion_type,
 *     ),
 *     ...
 *   );
 *
 * @param plan_id int
 * @param paddock_id int
 *
 * @return array where sday and eday are the number of days from start of plan
 *   for the exclusion start date and end date or zero if before plan start
 */
function farm_grazing_get_paddock_exclusions($plan_id, $paddock_id) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // load the plan dates
  list($p_start, $p_end) = farm_grazing_get_farm_plan_dates_from_id($plan_id, 'Y-m-d');

  // get query paddock exclusions
  $records = db_query('SELECT * FROM {farm_grazing_paddock_exclusions} WHERE paddock_id=:paddock_id', array(':paddock_id' => $paddock_id));

  $results = array();
  foreach ($records as $record) {

    // Get the exclusion start_date as YYYY-MM-DD.
    $e_start = date('Y-m-d', $record->start_date);

    // Get the exclusion end_date as YYYY-MM-DD.
    $e_end = date('Y-m-d', $record->end_date);

    // exclusion start_date as days from start of plan
    $sday = farm_grazing_get_days_between_dates($p_start, $e_start);

    // exclusion start_date as days from start of plan
    $eday = farm_grazing_get_days_between_dates($p_start, $e_end);

    $results[] = array(
      'id' => $record->paddock_id,
      'sday' => $sday,
      'eday' => $eday,
      'type' => $record->exclusion_type,
    );
  }

  return $results;
}

/**
 * farm_grazing_get_paddock_recovery($plan_id, $paddock_id)
 *
 *   $recovery = array(
 *     array(
 *       'sday' => (rec_start - plan_start) as days,
 *       'eday' => (rec_end   - plan_start) as days,
 *     ),
 *     ...
 *   );
 */
function farm_grazing_get_paddock_recovery($plan_id, $paddock_id, $end_date) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // load the plan dates
  list($p_start, $p_end) = farm_grazing_get_farm_plan_dates_from_id($plan_id, 'Y-m-d');
  list($yr, $mon, $day) = explode('-', $end_date);

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();

  if ($grazing_growing_season) {
    $recovery = db_query('SELECT (min_recovery+max_recovery)/2.0 AS avg_recovery FROM {farm_grazing_plan_recovery} WHERE plan_id=:plan_id AND month=:month', array(':plan_id' => $plan_id, ':month' => $yr . $mon))->fetchField();
  }
  else {
    $start_ts = $wrapper->field_farm_date_range->value->value();
    $end_ts = $wrapper->field_farm_date_range->value2->value();
    $plan_days = floor(($end_ts - $start_ts) / 3600 / 24);

    $expected_rotations = $wrapper->field_expected_rotations->value();
    $recovery = $plan_days / $expected_rotations;
  }

  // sday = end_date - plan start
  $sday = farm_grazing_get_days_between_dates($p_start, $end_date);

  // eday = sday + recovery
  $eday = $sday + $recovery;

  $results = array(
    array(
      'sday' => $sday,
      'eday' => $eday,
    ),
  );

  return $results;
}
