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

  $wrapper = entity_metadata_wrapper('farm_plan', $plan);
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

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
      t('You can define animals ') . l(t('HERE'), 'farm/asset/add/animal') . t('and assign them to a group. See the Help for more information.') .
      '</p>' .
      ''),
  );

  // get a list of herds for select
  $query = db_query("SELECT id, name FROM {farm_asset} WHERE type='group' AND active=1 ORDER BY name");

  $herd_options = array();

  $records = $query->fetchAll();

  foreach ($records as $record) {
    $herd_options[$record->id] = $record->name;
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
    '#required' => TRUE,
  );
  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add Herd to Plan'),
    '#id' => 'add-herd',
    '#submit' => array('farm_grazing_plan_herds_form_submit'),
    '#validate' => array('farm_grazing_plan_herds_form_validate'),
  );

  $query = db_query('SELECT * FROM {farm_grazing_herds} WHERE plan_id=:plan_id', array(':plan_id' => $plan));

  // table for herd summary
  $form['section_1']['title'] = array(
    '#markup' => '<h4>' . t('Summary of Herds in Plan') . '</h4>',
  );

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
    $summary = getHerdDataAndSummary($record->herd_id, $plan, $data);

    // save all the herd info here, we'll use it later
    // to generate the tables we display
    $herd_details[] = array(
      'herd_id' => $record->herd_id,
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
    $form['section_1']['herds'][$record->herd_id] = array(
      'name' => array(
        '#markup' => $group->name,
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
        '#markup' => l(t('Remove'), 'farm/grazing/plan/' . $plan . '/herd/' . $record->herd_id . '/edit', array('query' => array('destination' => current_path()))),
      ),
    );
  }

  // --------- Add table of herd details for each herd -----------------

  // ['herd_details'][herd_id]['title']
  // ['herd_details'][herd_id]['animals'][asset_id][fields]

  $animal_types = farm_grazing_animal_types();
  $type_options = array();
  foreach ($animal_types as $animal_type) {
    $type_options[$animal_type['type_id']] = $animal_type['name'];
  }

  foreach ($herd_details as $this_herd) {

    $form['herd_details'][$this_herd['herd_id']]['title'] = array(
      '#prefix' => '<h4>',
      '#markup' => t('Herd Details for ') . $this_herd['herd_name'],
      '#suffix' => '</h4>',
    );

    foreach ($this_herd['herd_data'] as $rec) {
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['name'] = array(
        '#markup' => $rec['name'],
      );
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['type'] = array(
        '#type' => 'select',
        '#options' => $type_options,
        '#default_value' => getAnimalTypeName($rec['type']),
      );
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['head_count'] = array(
        '#type' => 'textfield',
        '#default_value' => $rec['head_count'],
        '#size' => 30,
        '#maxlength' => 30,
      );
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['weight'] = array(
        '#type' => 'textfield',
        '#default_value' => $rec['weight'],
        '#size' => 30,
        '#maxlength' => 30,
      );
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['sau'] = array(
        '#markup' => round($rec['weight'] / 1000.0, 2),
      );
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['dmi'] = array(
        '#markup' => round($rec['head_count'] * $rec['weight'] * $rec['dmi_factor'], 2),
      );
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['arrival'] = array(
        '#type' => 'date_popup',
        '#date_format' => 'M j Y',
        '#date_type' => DATE_FORMAT_UNIX,
        '#date_year_range' => '-10:+3',
        '#date_label_position' => 'none',
        '#default_value' => $rec['arrival'],
      );
      $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']]['departure'] = array(
        '#type' => 'date_popup',
        '#date_format' => 'M j Y',
        '#date_type' => DATE_FORMAT_UNIX,
        '#date_year_range' => '-10:+3',
        '#date_label_position' => 'none',
        '#default_value' => $rec['departure'],
      );
    }
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#id' => 'save-changes',
    '#value' => t('Save Changes'),
    '#submit' => array('farm_grazing_plan_herds_form_submit'),
    '#validate' => array('farm_grazing_plan_herds_form_validate'),
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

  $header = array('Herd');  // don't t() this, it gets done in theme_

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
    $totals[] = $sum;
    if ($sum > $peak_sau) {
      $peak_sau = $sum;
    }
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

  $paddocks_in_plan = get_selected_paddock_quality_for_plan($plan);
  $cell_size = 0;
  foreach ($paddocks_in_plan as $paddock_id => $quality) {
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
    $rate = round($cell_size / $peak_sau, 1);
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

  $data = array();
  if (farm_grazing_validate_herd($herd_id, $plan_id, $data)) {
    form_set_error('herd_id', t('Please fix the issues listed before adding this herd to the plan.'));
  }

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

/**
 * theme_farm_grazing_plan_herds_form
 */
function theme_farm_grazing_plan_herds_form(&$vars) {
  $form = $vars['form'];

  // debug to see if theme_* is getting called
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, "Hello\n");


  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['text']);
  $output .= drupal_render($form['add']);

  $output .= drupal_render($form['section_1']['title']);
  $rows = array();
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
  foreach (element_children($form['herd_details']) as $herd_id) {
    // render table titles
    $output .= drupal_render($form['herd_details'][$herd_id]['title']);
    $table_id = 'herd_details_' . $herd_id;

    // assemble the table rows
    $rows = array();
    foreach (element_children($form['herd_details'][$herd_id]['animals']) as $aid) {
      $item =& $form['herd_details'][$herd_id]['animals'][$aid];

      // Assemble the row.
      $rows[] = array(
        'data' => array(
          drupal_render($item['name']),
          drupal_render($item['type']),
          drupal_render($item['head_count']),
          drupal_render($item['weight']),
          drupal_render($item['sau']),
          drupal_render($item['dmi']),
          drupal_render($item['arrival']),
          drupal_render($item['departure']),
        ),
      );
    }

    // define the table
    $header = array(
      t('Herd Name'),
      t('Type'),
      t('Head Count'),
      t('Weight'),
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

  // render SAU by Month table

  // render table titles
  $output .= drupal_render($form['section_3']['title']);

  $header = array();
  $rows = array();
  $first = TRUE;
  foreach (element_children($form['section_3']['herds']) as $herd_id) {
    // assemble the table rows
    $data = array();

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
    $rows[] = array('data' => $data);
    $first = FALSE;
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
