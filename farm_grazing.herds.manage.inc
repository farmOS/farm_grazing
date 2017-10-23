<?php

include_once "farm_grazing.utils.inc";

/**
 * Herd page callback.
 */
function farm_grazing_plan_herds_form($form, &$form_state, $plan_obj) {

  // Page layout
  //   button to add a herd to plan
  //     validate that herd has weigh, planned arrival, planned departure
  //     check is herd is part of another plan
  //   list summary table for each herd
  //   foreach herds in plan
  //       list herd details in a table
  //   SAU by Month table
  //   Summary information
  //     Total cell size
  //     peak month SAU
  //     Stocking rate
  //   click on herd summary to edit/delete it

  // get the plan id from the plan object
  $plan = $plan_obj->id;

  // Set the page title.
  drupal_set_title(t('Manage Herds for Plan'));

  $form['#tree'] = true;

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  // Return markup.
  $form['text'] = array(
    '#markup' => t('<p>' .
      t('A herd is defined as a group of animals that will be rotated between paddocks together regardless of the make up of animals in the herd.') .
      '</p><p>' .
      t('Herds are defined in farmOS as groups under the "Assets -> Groups" menu, then they can be selected here. You can also add to a herd, combine herds, split some animals out of a herd, or ship animals out of the plan.') .
      '</p></p>' .
      t('Click on the herd name to remove it from the plan.') .
      '</p>' .
      ''),
  );

  // get a list of herds for select
  $query = db_query("SELECT id, name FROM {farm_asset} WHERE type='group' AND active=1 ORDER BY name");

  $herd_options = array();

  $records = $query->fetchAll();

  foreach($records as $record) {
    $herd_options[$record->id] = $record->name;
  }

  // Form for adding a new exclusion.
  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add a herd'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['herd_id'] = array(
    '#type' => 'select',
    '#title' => t('Herd'),
    '#description' => t('Select the herd you want to add.'),
    '#options' => $herd_options,
    '#required' => TRUE,
  );
  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add Herd to Plan'),
    '#submit' => array('farm_grazing_plan_herds_form_submit'),
    '#validate' => array('farm_grazing_plan_herds_form_validate'),
  );

  $table = array(
    'header' => array(
      t('Herd Name'),
      t('Head Count'),
      t('SAU'),
      t('Daily Intake'),
      t('Earliest Arrival'),
      t('Latest Departure'),
    ),
    'rows' => array(),
    'empty' => t('No herds found in plan'),
  );

  $query = db_query('SELECT * FROM {farm_grazing_herds} WHERE plan_id=:plan_id', array(':plan_id' => $plan));

  // collect information about herds here
  $herd_details = array();

  $records = $query->fetchAll();
  foreach ($records as $record) {

    if (empty($record->herd_id)) {
      continue;
    }

    // load the asset so we have its name
    $group = farm_asset_load($record->herd_id);

    $data = array();
    $summary = getHerdDataAndSummary($record->herd_id, $data);

    // save all the herd info here, we'll use it later
    // to generate the tables we display
    $herd_details[] = array(
      'herd_name' => $group->name,
      'herd_data' => $data,
      'summary' => $summary,
    );

    //$format = 'Y-m-d';
    //$arrival_date = date($format, $herd['arrival']);
    //$departure_date = date($format, $herd['departure']);
    $arrival_date = $summary['arrival'];
    $departure_date = $summary['departure'];

    // build the section 1 summary table
    $table['rows'][] = array(
      '<a href="/farm/grazing/plan/' . $plan . '/herd/' . $record->herd_id .
        '/edit?destination=farm/plan/' . $plan . '/herds">' .
        $group->name . '</a>',
      $summary['head_count'],
      $summary['herd_sau'],
      $summary['herd_dmi'],
      $arrival_date,
      $departure_date,
    );
  }

  // table for herd summary
  $form['section_1'] = array(
    '#markup' => '<h4>' . t('Summary of Herds in Plan') . '</h4>',
  );
  $form['section_1_table'] = array(
    '#markup' => theme('table', $table),
  );

  // Add table of herd details for each herd
  $cnt = 0;
  foreach ($herd_details as $this_herd) {
    // define the table
    $table = array(
      'header' => array(
        t('Herd Name'),
        t('Type'),
        t('Head Count'),
        t('SAU'),
        t('Daily Intake'),
        t('Planned Arrival'),
        t('Planned Departure'),
      ),
      'rows' => array(),
      'empty' => t('No animals found in herd'),
    );

    foreach ($this_herd['herd_data'] as $rec) {
      $table['rows'][] = array(
        $rec['name'],
        getAnimalTypeName($rec['type']),
        $rec['head_count'],
        $rec['weight']/1000.0,
        $rec['weight']*$rec['dmi_factor']/100.0,
        $rec['arrival'],
        $rec['departure'],
      );
    }   

    $cnt++;
    $form['section_2.' . $cnt] = array(
      '#markup' => '<h4>' . t('Herd Details for ') . $this_herd['herd_name'] . '</h4>',
    );

    $form['section_2.'.$cnt.'_table'] = array(
      '#markup' => theme('table', $table),
    );
  }

  // get the start date month and year
  $start_date = new DateTime();
  $start_date->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value']);

  // get the end date month and year
  $end_date = new DateTime();
  $end_date->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value2']);

  // get the DateInterval between them
  $diff = $start_date->diff($end_date);

    // define the table
    $table = array(
      'header' => array(t('Herd')),
      'rows' => array(),
      'empty' => t('No data found for herds'),
    );

  // generate array of months between start and end
  $num_months = $diff->y * 12 + $diff->m;
  $date = $start_date;

  // generate the header text
  $helper = array();
  for( $i = 0; $i <= $num_months; $i++ ) {
    $key = $date->format('Ym');
    $helper[$key] = $i;

    $table['header'][] = $date->format('M Y');

    // increment the month
    $date->modify('+1 month');
  }

  // for each herd, cycle through months in plan
  // and sum up total SAUs by month
  $rows = array();
  $num_plan_months = $num_months;
  foreach ($herd_details as $this_herd) {
    // initial row for this herd
    $row = array();
    for( $i = 0; $i <= $num_plan_months; $i++ ) {
      $row[] = 0;
    }
    foreach ($this_herd['herd_data'] as $rec) {
      $sau = $rec['head_count']*$rec['weight']/1000.0;
      $start_date = date_create_from_format('Y-m-d', $rec['arrival']);
      $end_date = date_create_from_format('Y-m-d', $rec['departure']);

      // get the DateInterval between them
      $diff = $start_date->diff($end_date);

      // generate array of months between start and end
      $num_months = $diff->y * 12 + $diff->m;
      $date = $start_date;

      // sum up the sau by month
      for( $i = 0; $i <= $num_months; $i++ ) {
        $key = $date->format('Ym');
        if (isset($helper[$key])) {
          $row[$helper[$key]] = $row[$helper[$key]] + $sau;
        }

        // increment the month
        $date->modify('+1 month');
      }
    }
    array_unshift($row, $this_herd['herd_name']);
    $rows[] = $row;
  }

  // sum the columns in each row to get totals
  $peak_sau = 0;
  $totals = array(t('Total SAU'));
  for( $i = 0; $i <= $num_plan_months; $i++ ) {
    $sum = 0;
    foreach ($rows as $row) {
      if (isset($row[$i+1])) {
        $sum = $sum + $row[$i+1];
      }
    }
    $totals[] = $sum;
    if ($sum > $peak_sau) {
      $peak_sau = $sum;
    }
  }

  $rows[] = $totals;
  $table['rows'] = $rows;

  // Add table SAU by Plan Month for each herd
  $form['section_3'] = array(
    '#markup' => '<h4>' . t('SAU by Month') . '</h4>',
  );
  $form['section_3_table'] = array(
    '#markup' => theme('table', $table),
  );

  $paddocks_in_plan = get_selected_paddock_quality_for_plan($plan);
  $cell_size = 0;
  foreach($paddocks_in_plan as $paddock_id => $quality) {
    $cell_size = $cell_size + farm_area_calculate_area($paddock_id);
  }

  // Add Stocking Rates
  $form['section_4'] = array(
    '#markup' => '<h4>' . t('Stocking Rates') . '</h4>',
  );
  $form['section_4_cell_size'] = array(
    '#markup' => '<p>' . t('Total Cell Size: ') . round($cell_size, 1) . '</p>',
  );
  $form['section_4_peak_sau'] = array(
    '#markup' => '<p>' . t('Peak Monthly SAU: ') . $peak_sau . '</p>',
  );
  if ($peak_sau == 0) {
    $rate = '-';
  }
  else {
    $rate = round($cell_size/$peak_sau, 1);
  }
  $form['section_4_stocking_rate'] = array(
    '#markup' => '<p>' . t('Stocking Rate: ') . $rate . '</p>',
  );

  // Return the form.
  return $form;
}

/**
 * farm_grazing_plan_herds_form_submit()
 *
 * Add a herd to a plan
 */
function farm_grazing_plan_herds_form_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['values'], true));

  $plan_id = $form_state['values']['plan'];
  $herd_id = $form_state['values']['add']['herd_id'];

  db_merge('farm_grazing_herds')
    ->key(array(
      'plan_id' => $plan_id,
      'herd_id' => $herd_id,
    ))
    ->fields(array(
      'plan_id' => $plan_id,
      'herd_id' => $herd_id,
    ))
    ->execute();
}

/**
 *
 */
function farm_grazing_plan_herds_form_validate($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['values'], true));
  //fputs($fp, print_r($form_state['input'], true));

  $plan_id = $form_state['values']['plan'];
  $herd_id = $form_state['input']['add']['herd_id'];

  if (farm_grazing_validate_herd($herd_id)) {
    form_set_error('herd_id', t('Please fix the issues listed before adding this herd to the plan.'));
  }

}

/**
 * farm_grazing_validate_herd($herd_id)
 *
 * when a herd is attached to a grazing plan or otherwise modified
 * this function checks that all components of the herd have required
 * properties and initial observations.
 *
 * it should return null if ok, otherwise a document enumerating the
 * problems with links to where the user can fix them.
 *
 */
function farm_grazing_validate_herd($herd_id) {
  // TODO farm_grazing_validate_herd($herd_id) needs to support membership

  $errors = 0;

  // load the asset
  $herd = farm_asset_load($herd_id);

  // Things to validate
  //  farm_grazing_animal_type property exists and is valid
  $type = farm_asset_property_get($herd_id, 'farm_grazing_animal_type');
  if (empty($type)) {
    drupal_set_message(t('Herd') . '(<a href="/farm/asset/' . $herd_id . '/edit">' .
    $herd->name .
    '</a>)' .t(' needs its Animal Type property set.'), 'error');
    $errors++;
  }

  //  farm_grazing_planned_arrival property exists and is valid
  $arrival = farm_asset_property_get($herd_id, 'farm_grazing_planned_arrival');
  if (empty($arrival)) {
    drupal_set_message(t('Herd') . '(<a href="/farm/asset/' . $herd_id . '/edit">' .
    $herd->name .
    '</a>)' .t(' needs its Planned Arrival date set.'), 'error');
    $errors++;
  }

  //  farm_grazing_planned_departure property exists and is valid
  $departure = farm_asset_property_get($herd_id, 'farm_grazing_planned_departure');
  if (empty($departure)) {
    drupal_set_message(t('Herd') . '(<a href="/farm/asset/' . $herd_id . '/edit">' .
    $herd->name .
    '</a>)' .t(' needs its Planned Departure date set.'), 'error');
    $errors++;
  }

  //  has weight
  $data = farm_grazing_asset_weight($herd);
  if (empty($data) || empty($data['value'])) {
    drupal_set_message(t('Herd') . '(<a href="/farm/asset/' . $herd_id . '">' .
      $herd->name .
      '</a>)' .t(' needs an Observation -> Quantity -> weight set.'), 'error');
    $errors++;
  }

  //  TODO are all animals/groups are in some paddock

  //  TODO is asset attached to another active plan?

  return $errors;
}

/**
 * farm_grazing_plan_herd_edit_form()
 *
 * Remove the requested herd from the plan
 */
function farm_grazing_plan_herd_edit_form($form, &$form_state, $plan_obj, $herd_id) {

  // get plan name
  $plan_name = $plan_obj->name;

  // get herd name
  $herd = farm_asset_load($herd_id);
  $herd_name = $herd->name;

  $form['plan_id'] = array(
    '#type' => 'value',
    '#value' => $plan_obj->id,
  );

  $form['herd_id'] = array(
    '#type' => 'value',
    '#value' => $herd->id,
  );

  $form['edit']['text'] = array(
    '#markup' => '<h2>' . t('Confirm that you want to remove ') .
      '<b>' . $herd_name . '</b>' .
      t(' from plan ') . '<b>' . $plan_name . '</b>.</h2>',
  );
  $form['edit']['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#submit' => array('farm_grazing_herd_edit_form_submit'),
  );
  $form['edit']['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Remove Herd from Plan'),
    '#submit' => array('farm_grazing_herd_edit_form_submit'),
  );

  // Return the form.
  return $form;
}

/**
 * farm_grazing_herd_form_submit
 *
 * Handle farm_grazing_plan_herd_edit_form() submit processing
 */
function farm_grazing_herd_edit_form_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state, true));

  $plan_id = $form_state['values']['plan_id'];
  $herd_id = $form_state['values']['herd_id'];

  // if user clicked Cancel, get out of here
  if ($form_state['clicked_button']['#id'] == 'edit-cancel') {
    return;
  }

  if ($form_state['clicked_button']['#id'] == 'edit-delete') {
    db_delete('farm_grazing_herds')
      ->condition('plan_id', $plan_id)
      ->condition('herd_id', $herd_id)
      ->execute();
  }

}

