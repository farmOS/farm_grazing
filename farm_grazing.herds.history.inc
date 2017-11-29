<?php

/**
 * History page callback.
 */
function farm_grazing_plan_history_form($form, $form_state, $plan_obj = NULL) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // get list of paddocks in plan or all paddocks
  // looking back 1, 3, 5, 7, 9, all years

  // foreach paddock
  //   get historical moves in an out of paddock
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
      $paddock_ids = get_selected_paddock_for_plan($plan);
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
          countMonths($arrival, $departure, $row, $threshold);

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
  }

  $form['section_title'] = array(
    '#markup' => '<h4>' . t('Historical Grazing Patterns') . '</h4>',
  );
  $form['section_table'] = array(
    '#prefix' => '<div id="farm-grazing-history">',
    '#markup' => theme('table', $table),
    '#suffix' => '</div>',
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
  return $form['section_table'];
}
