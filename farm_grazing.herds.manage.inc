<?php

/**
 * @file
 * Grazing herd management form.
 */

/**
 * Herd page callback.
 */
function farm_grazing_plan_herds_form($form, &$form_state, $plan_obj) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // Add herd manage CSS.
  drupal_add_css(drupal_get_path('module', 'farm_grazing') . '/css/herds.manage.css');

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

  $wrapper = entity_metadata_wrapper('farm_plan', $plan);
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  // Set the page title.
  drupal_set_title(t('Manage Herds for Plan'));

  $form['#tree'] = TRUE;

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  // Return markup.
  $form['text'] = array(
    '#markup' => t('<p>' .
      t('A herd is defined as a group of animals that will be rotated between paddocks together regardless of the make up of animals in the herd.') .
      '</p><p>' .
      t('You can define animals <a href="@add_animal">HERE</a> and assign them to a group. See the Help for more information.', array('@add_animal' => url('farm/asset/add/animal'))) .
      '</p>' .
      ''),
  );

  // Get a list of all herds in the plan.
  $plan_herds = array();
  $result = db_query('SELECT * FROM {farm_grazing_herds} WHERE plan_id=:plan_id', array(':plan_id' => $plan));
  foreach ($result as $row) {
    if (!empty($row->herd_id)) {
      $plan_herds[$row->herd_id] = $row;
    }
  }

  // Build a list of other herds to select from (excluding ones that are
  // are already in the plan.
  $herd_options = array();
  $result = db_query("SELECT id, name FROM {farm_asset} WHERE type='group' AND archived IS NULL ORDER BY name");
  foreach ($result as $row) {
    if (!empty($row->id) && !array_key_exists($row->id, $plan_herds)) {
      $herd_options[$row->id] = $row->name;
    }
  }

  // Form for adding a herd to a plan
  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add a herd to the plan'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['herd_id'] = array(
    '#type' => 'select',
    '#title' => t('Herd'),
    '#description' => t('Select the herd you want to add.'),
    '#options' => $herd_options,
  );
  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add Herd to Plan'),
    '#validate' => array('farm_grazing_plan_herds_add_validate'),
    '#submit' => array('farm_grazing_plan_herds_add_submit'),
  );

  // table for herd summary
  $form['section_1']['title'] = array(
    '#markup' => '<h4>' . t('Summary of Herds in Plan') . '</h4>',
  );

  // collect information about herds here
  $herd_details = array();

  // Iterate through the herds in the plan.
  foreach ($plan_herds as $herd) {

    if (empty($herd->herd_id)) {
      continue;
    }

    // load the asset so we have its name
    $group = farm_asset_load($herd->herd_id);

    $data = array();
    $summary = farm_grazing_get_herd_data_and_summary($herd->herd_id, $plan, $data);

    // save all the herd info here, we'll use it later
    // to generate the tables we display
    $herd_details[] = array(
      'herd_id' => $herd->herd_id,
      'herd_name' => $group->name,
      'herd_data' => $data,
      'summary' => $summary,
    );

    // if either are empty then use the plan start or end as appropriate
    $arrival_date = $summary['arrival'];
    if (empty($arrival_date)) {
      $arrival_date = date('Y-m-d', $start_ts);
    }
    $departure_date = $summary['departure'];
    if (empty($departure_date)) {
      $departure_date = date('Y-m-d', $end_ts);
    }

    // build the section 1 summary table
    $form['section_1']['herds'][$herd->herd_id] = array(
      'name' => array(
        '#markup' => l($group->name, 'farm/asset/' . $herd->herd_id, array('query' => array('destination' => current_path()))),
      ),
      'head_count' => array(
        '#markup' => $summary['head_count'],
      ),
      'herd_sau' => array(
        '#markup' => $summary['herd_sau'],
      ),
      'herd_dmi' => array(
        '#markup' => $summary['herd_dmi'],
      ),
      'arrival_date' => array(
        '#markup' => $arrival_date,
      ),
      'departure_date' => array(
        '#markup' => $departure_date,
      ),
      'remove' => array(
        '#markup' => l(t('Remove'), 'farm/plan/' . $plan . '/herds/' . $herd->herd_id . '/remove', array('query' => array('destination' => current_path()))),
      ),
    );
  }

  // --------- Add table of herd details for each herd -----------------

  $animal_types = farm_grazing_animal_types();
  $type_options = array('' => '');
  foreach ($animal_types as $animal_type) {
    $type_options[$animal_type['type_id']] = $animal_type['name'];
  }

  // Track whether or not the animal type or weight are missing from any
  // animals, because this will affect SAU and DMI calculations.
  $missing_animal_data = array();

  foreach ($herd_details as $this_herd) {

    $form['herd_details'][$this_herd['herd_id']]['title'] = array(
      '#prefix' => '<h4>',
      '#markup' => t('Herd Details for ') . $this_herd['herd_name'],
      '#suffix' => '</h4>',
    );

    foreach ($this_herd['herd_data'] as $rec) {

      // Create an array for the animal fields, and a reference to it.
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']] = array();
      $animal_fields = &$form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']];

      // Animal record name.
      $animal_fields['name'] = array(
        '#markup' => l($rec['name'], 'farm/asset/' . $rec['id'], array('query' => array('destination' => current_path()))),
      );

      // Animal type.
      $animal_fields['type'] = array(
        '#title' => t('Animal type'),
        '#title_display' => 'invisible',
        '#type' => 'select',
        '#options' => $type_options,
        '#default_value' => !empty($rec['type']) ? $rec['type'] : '',
      );

      // Head count.
      $animal_fields['head_count'] = array(
        '#title' => t('Head count'),
        '#title_display' => 'invisible',
        '#type' => 'textfield',
        '#default_value' => $rec['head_count'],
        '#size' => 30,
        '#maxlength' => 30,
        '#element_validate' => array('element_validate_number'),
      );

      // Weight.
      $animal_fields['weight'] = array(
        '#title' => t('Average weight'),
        '#title_display' => 'invisible',
        '#type' => 'textfield',
        '#default_value' => $rec['weight'],
        '#size' => 30,
        '#maxlength' => 30,
        '#element_validate' => array('element_validate_number'),
      );

      $animal_fields['total_weight'] = array(
        '#markup' => $rec['weight'] * $rec['head_count'],
      );

      // SAU.
      $animal_fields['sau'] = array(
        '#markup' => round($rec['weight'] * $rec['head_count'] / 1000.0, 2),
      );

      // DMI.
      $animal_fields['dmi'] = array(
        '#markup' => round($rec['head_count'] * $rec['weight'] * $rec['dmi_factor'], 2),
      );

      // Planned arrival.
      $animal_fields['arrival'] = array(
        '#title' => t('Planned arrival'),
        '#title_display' => 'invisible',
        '#type' => 'date_popup',
        '#date_format' => 'M j Y',
        '#date_type' => DATE_FORMAT_UNIX,
        '#date_year_range' => '-10:+3',
        '#date_label_position' => 'none',
        '#default_value' => $rec['arrival'],
      );

      // Planned departure.
      $animal_fields['departure'] = array(
        '#title' => t('Planned departure'),
        '#title_display' => 'invisible',
        '#type' => 'date_popup',
        '#date_format' => 'M j Y',
        '#date_type' => DATE_FORMAT_UNIX,
        '#date_year_range' => '-10:+3',
        '#date_label_position' => 'none',
        '#default_value' => $rec['departure'],
      );

      // If the animal type or weight is empty...
      if (empty($rec['type']) || empty($rec['weight'])) {

        // Remember that this herd is missing data.
        $missing_animal_data[] = $this_herd['herd_id'];

        // If animal type is empty, add an error class to the dropdown.
        if (empty($rec['type'])) {
          $animal_fields['type']['#attributes']['class'] = array('error');
        }

        // If weight is empty, add an error class to the weight field, the SAU,
        // and the DMI.
        if (empty($rec['weight'])) {
          $animal_fields['weight']['#attributes']['class'] = array('error');
          $animal_fields['sau']['#prefix'] = '<span class="error">';
          $animal_fields['sau']['#suffix'] = '</span>';
          $animal_fields['dmi']['#prefix'] = '<span class="error">';
          $animal_fields['dmi']['#suffix'] = '</span>';
        }
      }
    }

    // If there was missing animal data, add an error class to the herd summary
    // SAU and DMI.
    if (in_array($this_herd['herd_id'], $missing_animal_data)) {
      $form['section_1']['herds'][$this_herd['herd_id']]['herd_sau']['#prefix'] = '<span class="error">';
      $form['section_1']['herds'][$this_herd['herd_id']]['herd_sau']['#suffix'] = '</span>';
      $form['section_1']['herds'][$this_herd['herd_id']]['herd_dmi']['#prefix'] = '<span class="error">';
      $form['section_1']['herds'][$this_herd['herd_id']]['herd_dmi']['#suffix'] = '</span>';
    }
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
  );

  // -- add SAU by Month table -------------------------------

  // get the start date month and year
  $start_date = new DateTime();
  $start_date->setTimestamp($start_ts);

  // get the end date month and year
  $end_date = new DateTime();
  $end_date->setTimestamp($end_ts);

  // get the DateInterval between them
  $diff = $start_date->diff($end_date);

  // generate array of months between start and end
  $num_months = $diff->y * 12 + $diff->m;
  $date = $start_date;

  $header = array('Herd');

  // generate the header text
  $helper = array();
  for ($i = 0; $i <= $num_months; $i++) {
    $key = $date->format('Ym');
    $helper[$key] = $i;

    $header[] = $date->format('M Y');

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
    for ($i = 0; $i <= $num_plan_months; $i++) {
      $row[] = 0;
    }
    foreach ($this_herd['herd_data'] as $rec) {
      $sau = $rec['head_count'] * $rec['weight'] / 1000.0;
      $start_date = date_create_from_format('Y-m-d', $rec['arrival']);
      $end_date = date_create_from_format('Y-m-d', $rec['departure']);

      // If the start/end date are empty, use the plan's start/end date.
      if (empty($start_date)) {
        $start_date = new DateTime();
        $start_date->setTimestamp($start_ts);
      }
      if (empty($end_date)) {
        $end_date = new DateTime();
        $end_date->setTimestamp($end_ts);
      }

      // get the DateInterval between them
      $diff = $start_date->diff($end_date);

      // generate array of months between start and end
      $num_months = $diff->y * 12 + $diff->m;

      // force the date to first day of month
      list($y, $m, $d) = explode(',', $start_date->format('Y,m,d'));
      $date = new DateTime();
      $date->setDate($y, $m, 1);

      // sum up the sau by month
      for ($i = 0; $i <= $num_months; $i++) {
        $key = $date->format('Ym');
        if (isset($helper[$key])) {
          $row[$helper[$key]] = $row[$helper[$key]] + $sau;

          // If there is missing data, add an error class.
          if (!empty($missing_animal_data)) {
            $row[$helper[$key]] = '<span class="error">' . $row[$helper[$key]] . '</span>';
          }
        }

        // increment the month
        $date->modify('+1 month');
      }
    }
    array_unshift($row, $this_herd['herd_name']);
    array_push($row, $this_herd['herd_id']);
    $rows[] = $row;
  }

  // sum the columns in each row to get totals
  $peak_sau = 0;
  $totals = array(t('Total SAU'));
  for ($i = 0; $i <= $num_plan_months; $i++) {
    $sum = 0;
    foreach ($rows as $row) {
      if (isset($row[$i + 1])) {
        $sum = $sum + $row[$i + 1];
      }
    }
    if ($sum > $peak_sau) {
      $peak_sau = $sum;
    }

    // If there is missing data, add an error class.
    if (!empty($missing_animal_data)) {
      $sum = '<span class="error">' . $sum . '</span>';
    }

    $totals[] = $sum;
  }

  // Push a dummy herd_id field.
  array_push($totals, -1);

  $rows[] = $totals;

  // Add table SAU by Plan Month for each herd
  $form['section_3']['title'] = array(
    '#prefix' => '<h4>',
    '#markup' => t('SAU by Month'),
    '#suffix' => '</h4>',
  );

  foreach ($rows as $row) {
    $herd_id = array_pop($row);
    $form['section_3']['herds'][$herd_id] = array();
    $i = 0;
    foreach ($row as $item) {
      $form['section_3']['herds'][$herd_id][$header[$i]] = array(
        '#markup' => $item,
      );
      $i++;
    }
  }

  $paddocks_in_plan = farm_grazing_get_selected_paddock_quality_for_plan($plan);
  $cell_size = 0;
  foreach ($paddocks_in_plan as $paddock_id => $quality) {
    $cell_size = $cell_size + farm_area_calculate_area($paddock_id);
  }

  // Add Stocking Rates
  $form['section_4'] = array(
    '#markup' => '<h4>' . t('Stocking Rates') . '</h4>',
  );

  // Total cell size.
  $total_cell_size = round($cell_size, 1);
  $form['section_4_cell_size'] = array(
    '#markup' => '<p>' . t('Total Cell Size: ') . $total_cell_size . '</p>',
  );

  // Peak SAU (if there is missing data, add an error class).
  $peak_sau_formatted = $peak_sau;
  if (!empty($missing_animal_data)) {
    $peak_sau_formatted = '<span class="error">' . $peak_sau_formatted . '</span>';
  }
  $form['section_4_peak_sau'] = array(
    '#markup' => '<p>' . t('Peak Monthly SAU: ') . $peak_sau_formatted . '</p>',
  );

  // Stocking rate (if there is missing data, add an error class).
  if ($peak_sau == 0) {
    $rate = '-';
  }
  else {
    $rate = round($cell_size / $peak_sau, 1);
  }
  if (!empty($missing_animal_data)) {
    $rate = '<span class="error">' . $rate . '</span>';
  }
  $form['section_4_stocking_rate'] = array(
    '#markup' => '<p>' . t('Stocking Rate: ') . $rate . '</p>',
  );

  // Return the form.
  return $form;
}

/**
 * Validation handler for adding a herd to a plan.
 */
function farm_grazing_plan_herds_add_validate($form, &$form_state) {

  // If a herd ID is not provided, flag an error.
  if (empty($form_state['values']['add']['herd_id'])) {
    form_set_error('add][herd_id', t('Please select a herd to add to the plan.'));
  }
}

/**
 * Submit handler for adding a herd to a plan.
 */
function farm_grazing_plan_herds_add_submit($form, &$form_state) {

  // Add the herd to the plan.
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
 * Validation handler for saving the herd information.
 */
function farm_grazing_plan_herds_form_validate($form, &$form_state) {

  // If there are no herd details, bail.
  if (empty($form_state['values']['herd_details'])) {
    return;
  }

  // Iterate through the herds.
  foreach ($form_state['values']['herd_details'] as $herd_id => $herd_details) {

    // If there are no animals in the herd, skip it.
    if (empty($herd_details['animals'])) {
      continue;
    }

    // Iterate through the animals.
    foreach ($herd_details['animals'] as $animal_id => $animal_details) {

      // Get the original $form array for this animal.
      $animal_form = $form['herd_details'][$herd_id]['animals'][$animal_id];

      // If certain fields are modified, make sure they are not left blank.
      $required_fields = array(
        'type' => t('Animal type'),
        'head_count' => t('Head count'),
        'weight' => t('Weight'),
        'arrival' => t('Planned arrival'),
        'departure' => t('Planned departure'),
      );
      foreach ($required_fields as $field => $label) {
        if ($animal_form[$field]['#default_value'] != $animal_details[$field]) {
          if (empty($animal_details[$field])) {
            form_set_error('herd_details][' . $herd_id . '][animals][' . $animal_id . '][' . $field, $label . ' ' . t('is required.'));
          }
        }
      }
    }
  }
}

/**
 * Submit handler for saving the herd information.
 */
function farm_grazing_plan_herds_form_submit($form, &$form_state) {

  // If there are no herd details, bail.
  if (empty($form_state['values']['herd_details'])) {
    return;
  }

  // Remember if values have been updated.
  $updated = FALSE;

  // Iterate through the herds.
  foreach ($form_state['values']['herd_details'] as $herd_id => $herd_details) {

    // If there are no animals in the herd, skip it.
    if (empty($herd_details['animals'])) {
      continue;
    }

    // Iterate through the animals.
    foreach ($herd_details['animals'] as $animal_id => $animal_details) {

      // Get the original $form array for this animal.
      $animal_form = $form['herd_details'][$herd_id]['animals'][$animal_id];

      // If the animal's type has changed...
      if ($animal_form['type']['#default_value'] != $animal_details['type']) {

        // Set the asset property value.
        farm_asset_property_set($animal_id, 'farm_grazing_animal_type', $animal_details['type']);
        $updated = TRUE;
      }

      // If the animal's head count has changed...
      if ($animal_form['head_count']['#default_value'] != $animal_details['head_count']) {

        // Create an inventory log.
        $animal = farm_asset_load($animal_id);
        if (!empty($animal)) {
          farm_inventory_set($animal, $animal_details['head_count']);
          $updated = TRUE;
        }
      }

      // If the animal's weight has changed...
      if ($animal_form['weight']['#default_value'] != $animal_details['weight']) {

        // Create a weight log.
        $animal = farm_asset_load($animal_id);
        if (!empty($animal)) {
          farm_livestock_weight_set($animal, $animal_details['weight'], 'lbs');
          $updated = TRUE;
        }
      }

      // If the animal's planned arrival has changed...
      if ($animal_form['arrival']['#default_value'] != $animal_details['arrival']) {

        // Set the asset property value.
        $planned_arrival = strtotime($animal_details['arrival']);
        farm_asset_property_set($animal_id, 'farm_grazing_planned_arrival', $planned_arrival);
        $updated = TRUE;
      }

      // If the animal's planned departure has changed...
      if ($animal_form['departure']['#default_value'] != $animal_details['departure']) {

        // Set the asset property value.
        $planned_departure = strtotime($animal_details['departure']);
        farm_asset_property_set($animal_id, 'farm_grazing_planned_departure', $planned_departure);
        $updated = TRUE;
      }
    }
  }

  // If any values were updated, show a message.
  if ($updated) {
    drupal_set_message(t('Animals were updated.'));
  }
}

/**
 * Form for removing a herd from the plan.
 */
function farm_grazing_plan_herd_remove_form($form, &$form_state, $plan_obj, $herd_id) {

  // Load the herd
  $herd = farm_asset_load($herd_id);

  // If the herd or plan are empty, show page not found.
  if (empty($plan_obj) || empty($herd)) {
    drupal_not_found();
    drupal_exit();
  }

  // Store the plan ID and herd ID for use in the submit function.
  $form['plan_id'] = array(
    '#type' => 'value',
    '#value' => $plan_obj->id,
  );
  $form['herd_id'] = array(
    '#type' => 'value',
    '#value' => $herd->id,
  );

  // Build a return path.
  $path = drupal_get_destination();
  if ($path['destination'] == current_path()) {
    $plan_uri = entity_uri('farm_plan', $plan_obj);
    $path = $plan_uri['path'] . '/';
  }

  // Build and return a confirmation form.
  return confirm_form($form,
    t('Are you sure you want to remove herd %herd_name from the %plan_name?', array('%herd_name' => $herd->name, '%plan_name' => $plan_obj->name)),
    $path,
    t('You can add it back later if you change your mind.'),
    t('Remove it!'),
    t('Cancel')
  );
}

/**
 * Submit handler for the herd removal form.
 */
function farm_grazing_plan_herd_remove_form_submit($form, &$form_state) {

  // If a plan ID and herd ID are not available, bail.
  if (empty($form_state['values']['plan_id']) || empty($form_state['values']['herd_id'])) {
    return;
  }

  // Delete the record from the {farm_grazing_herds} table.
  db_delete('farm_grazing_herds')
    ->condition('plan_id', $form_state['values']['plan_id'])
    ->condition('herd_id', $form_state['values']['herd_id'])
    ->execute();

  // Look up all rotations that reference this plan and herd, and delete logs.
  $result = db_query('SELECT log_id FROM {farm_grazing_rotations} WHERE plan_id = :plan_id AND herd_id = :herd_id', array('plan_id' => $form_state['values']['plan_id'], ':herd_id' => $form_state['values']['herd_id']));
  foreach ($result as $row) {
    if (!empty($row->log_id)) {
      entity_delete('log', $row->log_id);
    }
  }

  // Delete all rotations that reference this plan and herd.
  db_delete('farm_grazing_rotations')
    ->condition('plan_id', $form_state['values']['plan_id'])
    ->condition('herd_id', $form_state['values']['herd_id'])
    ->execute();

  // Tell the user what we did.
  drupal_set_message(t('A herd was removed from the plan. All rotations associated with the herd were deleted. Check your planned rotations and make any necessary adjustments.'), 'warning');
}

/**
 * theme_farm_grazing_plan_herds_form
 */
function theme_farm_grazing_plan_herds_form(&$vars) {
  $form = $vars['form'];

  // Add some CSS to hide the date descriptions.
  $css = '.form-type-date-popup .help-block {display:none;}';
  drupal_add_css($css, array('type' => 'inline'));

  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['text']);
  $output .= drupal_render($form['add']);

  $output .= drupal_render($form['section_1']['title']);
  $rows = array();
  if (!empty($form['section_1']['herds'])) {
    foreach (element_children($form['section_1']['herds']) as $herd_id) {
      $item =& $form['section_1']['herds'][$herd_id];
      $rows[] = array(
        'data' => array(
          drupal_render($item['name']),
          drupal_render($item['head_count']),
          drupal_render($item['herd_sau']),
          drupal_render($item['herd_dmi']),
          drupal_render($item['arrival_date']),
          drupal_render($item['departure_date']),
          drupal_render($item['remove']),
        ),
      );
    }
  }

  $header = array(
    t('Herd Name'),
    t('Head Count'),
    t('SAU'),
    t('Daily Intake'),
    t('Earliest Arrival'),
    t('Latest Departure'),
    t('Remove'),
  );

  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
    'empty' => t('No Herds Defined for this Plan.'),
    'attributes' => array('id' => 'herd_summary_table'),
  ));

  // render tables for herd details
  if (!empty($form['herd_details'])) {
    foreach (element_children($form['herd_details']) as $herd_id) {
      // render table titles
      $output .= drupal_render($form['herd_details'][$herd_id]['title']);
      $table_id = 'herd_details_' . $herd_id;

      // assemble the table rows
      $rows = array();
      if (!empty($form['herd_details'][$herd_id]['animals'])) {
        foreach (element_children($form['herd_details'][$herd_id]['animals']) as $aid) {
          $item =& $form['herd_details'][$herd_id]['animals'][$aid];

          // Assemble the row.
          $rows[] = array(
            'data' => array(
              drupal_render($item['name']),
              drupal_render($item['type']),
              drupal_render($item['head_count']),
              drupal_render($item['weight']),
              drupal_render($item['total_weight']),
              drupal_render($item['sau']),
              drupal_render($item['dmi']),
              drupal_render($item['arrival']),
              drupal_render($item['departure']),
            ),
          );
        }
      }

      // define the table
      $header = array(
        t('Animal Record'),
        t('Type'),
        t('Head Count'),
        t('Weight (Average)'),
        t('Total Weight'),
        t('SAU'),
        t('Daily Intake'),
        t('Planned Arrival'),
        t('Planned Departure'),
      );

      $output .= theme('table', array(
        'header' => $header,
        'rows' => $rows,
        'empty' => t('No animals found in herd'),
        'attributes' => array('id' => $table_id),
      ));
    }
  }

  // render SAU by Month table

  // render table titles
  $output .= drupal_render($form['section_3']['title']);

  $header = array();
  $rows = array();
  $first = TRUE;
  if (!empty($form['section_3']['herds'])) {
    foreach (element_children($form['section_3']['herds']) as $herd_id) {
      // assemble the table rows
      $data = array();

      if (!empty($form['section_3']['herds'][$herd_id])) {
        foreach (element_children($form['section_3']['herds'][$herd_id]) as $hdr) {
          $item =& $form['section_3']['herds'][$herd_id][$hdr];

          if ($first) {
            if ($hdr == 'Herd') {
              $header[] = t($hdr);
            }
            else {
              $header[] = $hdr;
            }
          }

          $data[] = drupal_render($item);
        }
      }
      $rows[] = array('data' => $data);
      $first = FALSE;
    }
  }

  $table = array(
    'header' => $header,
    'rows' => $rows,
    'empty' => t('No data or herds.'),
    'attributes' => array('id' => 'sau_by_month_table'),
  );

  $output .= theme('table', $table);

  // render any remaining elements
  $output .= drupal_render_children($form);

  return $output;
}
