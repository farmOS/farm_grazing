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
 * farm_grazing_plan_plot($plan_obj, $paddock_id, $start_date, $end_date)
 *
 * @param plan_obj   - drupal plan object
 * @param paddock_id - int, paddock id for this plan
 * @param start_date - string YYYY-MM-DD, start of paddock grazing
 * @param end_date   - string YYYY-MM-DD, end of paddock grazing
 *
 * @returns an image showing rotation in calendar or calendar header image
 */
function farm_grazing_plan_plot($plan_obj, $paddock_id, $start_date, $end_date) {
  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // Set the header Content-type
  drupal_add_http_header('Content-Type', 'image/svg+xml');

  // output the image
  $svg = farm_grazing_plan_plot_svg($plan_obj, $paddock_id, array(), $start_date, $end_date);
  echo $svg;

  drupal_exit();
}

/**
 * Generate SVG that plots a paddock rotation for visualization.
 *
 * @param plan_obj   - drupal plan object
 * @param paddock_id - int, paddock id for this plan
 * @param start_date - string YYYY-MM-DD, start of paddock grazing
 * @param end_date   - string YYYY-MM-DD, end of paddock grazing
 *
 * @return SVG text showing rotation in calendar or calendar header image
 */
function farm_grazing_plan_plot_svg($plan_obj, $paddock_id, $considerations, $start_date, $end_date) {
  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_obj->id);
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  $c_types = farm_plan_consideration_types();

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

  // Create an image.
  $im = '<?xml version="1.0" encoding="UTF-8" standalone="yes"?>' .
    '<svg xmlns="http://www.w3.org/2000/svg"' .
    ' xmlns:xlink= "http://www.w3.org/1999/xlink"' .
    ' width="' . $width . '" height="' . $height . '"' .
    ' viewBox="0 0 ' . $width . ' ' . $height . '">';

  if (!$header) {

    // add urgent concerns
    if (!empty($considerations)) {
        $color = $c_types['urgent']['color'];
        foreach ($considerations as $key => $records) {
            if (! ($key == 0 or $key == $paddock_id)) {
                continue;
            }
            foreach ($records as $rec) {
                if ($rec->type != 'urgent') {
                    continue;
                }
                $c_start = date('Y-m-d', $rec->start_time);
                $c_end = date('Y-m-d', $rec->end_time);
                $sday = farm_grazing_get_days_between_dates($plan_start, $c_start);
                $eday = farm_grazing_get_days_between_dates($plan_start, $c_end);
                $x1 = $sday * $day_width;
                $y1 = 0;
                $w = ($eday - $sday) * $day_width;
                $h = $height;
                $title = $rec->name;
                $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
                       "style=\"fill:$color;fill-opacity:1\" alt=\"$title\" class=\"enabled\" />";
            }
        }
    }

    // add management concerns
    if (!empty($considerations)) {
        $color = $c_types['concern']['color'];
        foreach ($considerations as $key => $records) {
            if (! ($key == 0 or $key == $paddock_id)) {
                continue;
            }
            foreach ($records as $rec) {
                if ($rec->type != 'concern') {
                    continue;
                }
                $c_start = date('Y-m-d', $rec->start_time);
                $c_end = date('Y-m-d', $rec->end_time);
                $sday = farm_grazing_get_days_between_dates($plan_start, $c_start);
                $eday = farm_grazing_get_days_between_dates($plan_start, $c_end);
                $x1 = $sday * $day_width;
                $y1 = 2;
                $w = ($eday - $sday) * $day_width;
                $h = $height - 4;
                $title = $rec->name;
                $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
                       "style=\"fill:$color;fill-opacity:.5\" alt=\"$title\" class=\"enabled\" />";
            }
        }
    }

    // add exclusion periods
    $color = $c_types['farm_grazing_exclusion']['color'];
    foreach ($exclusions as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 4;
      $w = ($event['eday'] - $event['sday']) * $day_width;
      $h = $height - 8;
      $title = $event['type'];
      $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
             "style=\"fill:$color;fill-opacity:.75\" alt=\"$title\" class=\"enabled\" />";
    }

    // add recovery periods
    $color = 'goldenrod';
    foreach ($recovery as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 8;
      $w = ($event['eday'] - $event['sday']) * $day_width;
      $h = $height - 16;
      $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
        "style=\"fill:$color;fill-opacity:.5\" alt=\"Grazing Recovery Period\" class=\"enabled\" />";
    }

    $delta = 0;
    if ($days_to_start == $days_to_end) {
      $delta = 1;
    }

    // add the paddock for grazing period
    $color = 'green';
    $x1 = $days_to_start * $day_width;
    $y1 = 12;
    // TODO check the math on (end-start) get rounded to zero??
    // +1 to days so something alsways should on plot
    $w = ($days_to_end - $days_to_start + $delta) * $day_width;
    $h = $height - 24;
    $im .= "<rect x=\"$x1\" y=\"$y1\" width=\"$w\" height=\"$h\" " .
      "style=\"fill:$color;fill-opacity:1\" alt=\"Grazing\" class=\"enabled\" />";
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
  $im .= '</svg>';

  // return the svg text
  return $im;
}

/**
 * farm_grazing_get_paddock_exclusions($plan_id, $paddock_id)
 *
 *   $exclusions = array(
 *     array(
 *       'id' => paddock_id,
 *       'sday' => (exc_start - plan_start) as days,
 *       'eday' => (exc_end   - plan_start) as days,
 *       'type' => exclusion_type, // name
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
  $records = db_query("SELECT e.entity_id as paddock_id,
      c.start_time as start_date,
      c.end_time as end_date,
      c.name as exclusion_type
      FROM {farm_plan_consideration} c JOIN {farm_plan_consideration_entity} e ON c.id=e.consideration_id
      WHERE c.type='farm_grazing_exclusion' AND e.entity_type='taxonomy_term' AND e.entity_id=:paddock_id",
      array(':paddock_id' => $paddock_id));

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

  $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $p_start = date('Y-m-d', $start_ts);
  $end_ts = $wrapper->field_farm_date_range->value2->value();
  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();

  $results = array();

  // get all plans that proceed or overlap this plan by some number of days
  $window = 90.0*24*3600;   // a 90 window
  $plans = db_query("SELECT p.id, p.name, d.field_farm_date_range_value AS p_start,
            d.field_farm_date_range_value2 AS p_end
       FROM {farm_plan} p JOIN {field_data_field_farm_date_range} d
         ON p.id=d.entity_id AND p.type=d.bundle AND d.entity_type = 'farm_plan'
      WHERE d.field_farm_date_range_value2 > :start_ts - :window
         OR p.id=:plan_id",
     array(':plan_id' => $plan_id, ':start_ts' => $start_ts, ':window' => $window));

  // get list of rotation end dates for this paddock in each of the plans as the recovery start date
  foreach ($plans as $plan) {
      $r_starts = db_query('SELECT start_date + duration*24*3600 AS r_start
          FROM {farm_grazing_rotations}
          WHERE plan_id=:plan_id AND paddock_id=:paddock_id',
        array(':plan_id' => $plan->id, ':paddock_id' => $paddock_id));

      // compute the recovery period for each rotation given the plan the rotation is in
      // and whether it is growing season or not

      foreach ($r_starts as $rec) {
          $wrap = entity_metadata_wrapper('farm_plan', $plan->id);
          $growing = $wrap->field_grazing_growing_season->value();

          if ($growing) {
            // get the year month of the start of recovery period
            list($yr, $mon, $day) = explode('-', date('Y-m-d', $rec->r_start));

            $recovery = db_query('SELECT (min_recovery+max_recovery)/2.0 AS avg_recovery FROM {farm_grazing_plan_recovery} WHERE plan_id=:plan_id AND month=:month', array(':plan_id' => $plan->id, ':month' => $yr . $mon))->fetchField();
          }
          else {
            // TODO
            $plan_days = floor(($end_ts - $start_ts) / 3600 / 24);

            $expected_rotations = $wrapper->field_expected_rotations->value();
            if ($expected_rotations == 0) {
                $expected_rotations = 1;
            }
            $recovery = $plan_days / $expected_rotations;
          }

          // sday = days from plan start to start of recovery period
          $sday = farm_grazing_get_days_between_dates($p_start, date('Y-m-d', $rec->r_start));

          // if the start of recovery is before plan start then recovery need to be adjusted also
          if ($sday == 0) {
            $delta = farm_grazing_get_days_between_dates(date('Y-m-d', $rec->r_start), $p_start);
            $recovery = $recovery - $delta;
          }

          // eday = sday + recovery
          $eday = $sday + $recovery;

          $results[] = array(
              'sday' => $sday,
              'eday' => $eday,
            );
      }
  }

  return $results;
}
