<?php

/**
 * @file
 * Grazing history form.
 */

/**
 * History page callback.
 */
function farm_grazing_plan_history_form($form, $form_state, $plan_obj = NULL) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // get list of paddocks in plan or all paddocks
  // looking back 1, 3, 5, 7, 9, all years

  // foreach paddock
  //   get historical moves in and out of paddock
  //   build row with count of use by month
  //   mon-year when last used

  // display as table

  $plan = $plan_obj->id;

  // Set the page title.
  drupal_set_title(t('Check historical grazing patterns'));

  // Return markup.
  $form['text'] = array(
    '#markup' => '<p>' . t('Select which paddocks and what time range you want to review. Review the results and factor this into your planning for ordering the herd rotations through the assigned paddocks.') . '</p>',
  );

  if (isset($plan)) {
    $form['plan_id'] = array(
      '#type' => 'value',
      '#value' => $plan,
    );
  }
  $form['select'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select Options for Report'),
    '#description' => t('Select which paddocks and how many years back to look.'),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $form['select']['paddocks'] = array(
    '#type' => 'select',
    '#title' => t('Which paddocks?'),
    '#description' => 'Select if you want to review all paddocks or only those in the plan.',
    '#options' => array(
      'all' => t('All paddocks'),
      'plan' => t('Only paddocks in plan'),
    ),
    '#required' => TRUE,
  );

  $form['select']['years'] = array(
    '#type' => 'select',
    '#title' => t('How many years?'),
    '#description' => t('Select how many years back to include in report. This will look backwards from the current date (@date).', array('@date' => date('M d, Y'))),
    '#options' => array(
      '1' => t('1 year'),
      '3' => t('3 years'),
      '5' => t('5 years'),
      '7' => t('7 years'),
      'all' => t('all years'),
    ),
    '#required' => TRUE,
  );

  $form['select']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update Page'),
    '#ajax' => array(
      'wrapper' => 'farm-grazing-history',
      'callback' => 'farm_grazing_plan_history_form_ajax',
    ),
  );

  // display table
  $table = array(
    'header' => array(
      t('Paddock'),
      t('Jan'),
      t('Feb'),
      t('Mar'),
      t('Apr'),
      t('May'),
      t('Jun'),
      t('Jul'),
      t('Aug'),
      t('Sep'),
      t('Oct'),
      t('Nov'),
      t('Dec'),
      t('Total'),
      t('Last Grazed'),
    ),
    'rows' => array(),
    'empty' => t('No paddocks found!'),
  );

  // build the second table showing starting and ending paddock
  // for each plan covered by the history range

  $table2 = array(
    'header' => array(
      t('Plan Name'),
      t('Herd Name'),
      t('Start Paddock'),
      t('Start Date'),
      t('End Paddock'),
      t('End Date'),
    ),
    'rows' => array(),
    'empty' => t('No data available'),
  );

  // If the form has been submitted, and paddocks and years are available...
  if (!empty($form_state['values']['paddocks']) && !empty($form_state['values']['years'])) {

    // Calculate the timestamp to look back to. If years is "all", set to NULL.
    if ($form_state['values']['years'] == 'all') {
      $start_time = NULL;
    }
    else {
      $start_time = REQUEST_TIME - ($form_state['values']['years'] * 365 * 24 * 60 * 60);
    }

    // Build a list of paddocks.
    $paddocks = array();
    if ($form_state['values']['paddocks'] == 'all') {
      $paddocks = farm_area_load_areas('paddock', 'name');
    }
    elseif ($form_state['values']['paddocks'] == 'plan') {
      $paddock_ids = farm_grazing_get_selected_paddock_for_plan($plan);
      $paddocks = taxonomy_term_load_multiple($paddock_ids);
    }

    // Iterate through the paddocks.
    $i = 0;
    foreach ($paddocks as $paddock) {

      // create array to hold our data
      //    [name, 12 months, total, last grazed]
      $name = l($paddock->name, url('taxonomy/term/' . $paddock_ids[$i]));
      $i++;
      $row = array($name, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '');

      $date_last_grazed = 0;

      // Query for the paddock's history of Group assets, going back to the
      // requested timestamp.
      $history = farm_movement_area_history($paddock, 'group', $start_time);

      // Iterate through the asset histories.
      foreach ($history as $asset_id => $movements) {

        // Check to make sure that the group is a herd.
        /**
         * @todo
         * Add an asset property to groups that identifies them as "herds",
         * and only include groups that are "herds" here.
         */

        // Each asset may have moved through this paddock more than once, so
        // iterate through each arrival/departure pair.
        foreach ($movements as $movement) {

          // Get the arrival time.
          $arrival = $movement['arrive']->timestamp;

          // Get the departure time, if available. If not available, assume the
          // group is still there.
          $departure = !empty($movement['depart']->timestamp) ? $movement['depart']->timestamp : REQUEST_TIME;

          // Calculate the number of days that the asset was in the paddock
          // for each month during the specified time span.
          // threshold is the number of days overlap in the month
          // for the month to be counted.
          $threshold = 15;
          farm_grazing_count_months($arrival, $departure, $row, $threshold);

          // get date last grazed
          if ($departure > $date_last_grazed) {
            $date_last_grazed = $departure;
          }
        }
      }

      // add last grazed to table
      $row[14] = date('Y-m-d', $date_last_grazed);

      // add data to table rows
      if ($date_last_grazed > 0) {
        $table['rows'][] = $row;
      }
    }
    // first get a list plan_id, herd_id, herd_name for for all plans
    // that meet the search criteria

    // rows2[plan_name, herd_name, paddock_start, start_date, paddock_end, end_date]
    $rows2 = array();

    $sql = 'SELECT h.plan_id, h.herd_id, p.name as plan_name, a.name as herd_name
      FROM {farm_plan} p
      LEFT JOIN {farm_grazing_herds} h ON p.id=h.plan_id
      LEFT JOIN field_data_field_farm_date_range d
        ON d.field_farm_date_range_value > :start_date
        OR d.field_farm_date_range_value2 > :start_date
      LEFT JOIN {farm_asset} a ON a.id=h.herd_id
      ORDER BY d.field_farm_date_range_value desc';

    // foreach record of above
    $query = db_query($sql, array(':start_date' => $start_time));
    $records = $query->fetchAll();
    foreach ($records as $record) {
      $row = array($record->plan_name, $record->herd_name);

      // get the first paddock of that rotation
      $sql2 = 'SELECT t.name, r.start_date
        FROM (
          SELECT plan_id, herd_id, min(start_date) AS min_start
          FROM {farm_grazing_rotations}
          GROUP BY plan_id, herd_id
        ) AS x JOIN {farm_grazing_rotations} AS r
        ON x.plan_id=r.plan_id AND x.herd_id=r.herd_id AND x.min_start=r.start_date
        LEFT JOIN {taxonomy_term_data} t ON r.paddock_id=t.tid
        WHERE x.plan_id=:plan_id AND x.herd_id=:herd_id';

      $results = db_query($sql2, array(':plan_id' => $record->plan_id, ':herd_id' => $record->herd_id))->fetchAssoc();
      $row[] = $results['name'];
      $row[] = !empty($results['start_date']) ? date('Y-m-d', $results['start_date']) : '';

      // get the last paddock of the rotation

      $sql3 = 'SELECT t.name, round(r.start_date + r.duration * 24 * 3600) AS end_date
        FROM (
          SELECT plan_id, herd_id, max(start_date) AS max_start
          FROM {farm_grazing_rotations}
          GROUP BY plan_id, herd_id
        ) AS x JOIN {farm_grazing_rotations} AS r
        ON x.plan_id=r.plan_id AND x.herd_id=r.herd_id AND x.max_start=r.start_date
        LEFT JOIN {taxonomy_term_data} t ON r.paddock_id=t.tid
        WHERE x.plan_id=:plan_id AND x.herd_id=:herd_id';

      $results = db_query($sql3, array(':plan_id' => $record->plan_id, ':herd_id' => $record->herd_id))->fetchAssoc();
      $row[] = $results['name'];
      $row[] = !empty($results['end_date']) ? date('Y-m-d', $results['end_date']) : '';

      $table2['rows'][] = $row;
    }

  }

  $form['tables'] = array(
    '#prefix' => '<div id="farm-grazing-history">',
    '#suffix' => '</div>',
  );

  $form['tables']['section_title'] = array(
    '#markup' => '<h4>' . t('Historical Grazing Patterns') . '</h4>',
  );
  $form['tables']['section_table'] = array(
    '#markup' => theme('table', $table),
  );

  // render the table

  $form['tables']['section2_title'] = array(
    '#markup' => '<h4>' . t('Historical Start/End Paddock Usage') . '</h4>',
  );
  $form['tables']['section2_table'] = array(
    '#markup' => theme('table', $table2),
  );

  return $form;
}

/**
 * Submit function for the grazing plan history form.
 */
function farm_grazing_plan_history_form_submit($form, &$form_state) {

  // Rebuild the form so that the table is regenerated.
  $form_state['rebuild'] = TRUE;
}

/**
 *
 */
function farm_grazing_plan_history_form_ajax($form, $form_state) {
  return $form['tables'];
}
