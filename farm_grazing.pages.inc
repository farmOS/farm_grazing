<?php

/**
 * @file
 * Grazing pages code.
 */

  //'page callback' => 'farm_grazing_plan_concerns_page',
  //'page callback' => 'drupal_get_form(farm_grazing_select_paddocks_form)',
  //'page callback' => 'drupal_get_form(farm_grazing_exclusions_form)',
  //'page callback' => 'drupal_get_form(farm_grazing_plan_productivity_form'),
  //'page callback' => 'drupal_get_form(farm_grazing_plan_recovery_form)',
  //'page callback' => 'farm_grazing_plan_summary_page',
  //'page callback' => 'drupal_get_form(farm_grazing_plan_herds_form)',
  //'page callback' => 'drupal_get_form(farm_grazing_plan_herds_paddocks_form)',
  //'page callback' => 'drupal_get_form(farm_grazing_plan_history_form)',
  //'page callback' => 'drupal_get_form(farm_grazing_plan_rotations_form)',
  //'page callback' => 'farm_grazing_plan_implement_page',
  //'page callback' => 'farm_grazing_plan_implement_status_page',


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

//-------------- Start of page/form callbacks --------------------

/**
 * Management concerns page callback.
 */
function farm_grazing_plan_concerns_page($plan_obj) {

  $plan = $plan_obj->id;

  // Set the page title.
  drupal_set_title(t('Management concerns'));

  // Return markup.
  return array(
    '#markup' => t('
      <p>In this section, think about various factors that influence your calendar for the plan. Consider:</p>
      <ul>
          <li>Livestock events like birthing, breeding, weaning, etc and when these are planned.
          <li>Management events such as vacations, family or community events that need to be planned around.
          <li>Other specific events like hunting seasons, parasites, or flooding that might influence the sequence of paddock moves.
      </ul>
      <p>Enter management events into the Calendar on the "Calendar" tab. Enter other events into your ') . '<a href="/farm/plan/' . $plan . '/edit">' . t('Planning Factors') . '</a>' . t(' for the plan for now.</p>
      <p>You will come back to this later when you need to enter paddock exclusions and are planning your moves as it will be helpful to be able to see these events while planning moves.</p>
    '),
  );
}

/**
 * Generate form with table of paddocks callback
 */
function farm_grazing_select_paddocks_form($form, &$form_state, $plan_obj) {
  drupal_set_title(t('Select Paddocks'));
  $form['text'] = array(
    '#markup' => t('Select which paddocks you want available for use in this plan.'),
  );

  // get the plan id from the plan object
  $plan = $plan_obj->id;

  $paddocks = farm_area_load_areas('paddock', 'name');

  $data = array();
  foreach ($paddocks as $paddock) {
    if (empty($paddock->tid)) {
      continue;
    }

    //$term = taxonomy_term_load($paddock->tid);

    $data[$paddock->tid] = array(
      $paddock->name,
      farm_area_calculate_area($paddock->tid),
    );
  }

  // get a list of selected paddocks
  $selected_paddocks = get_selected_paddock_for_plan($plan);

  // build the default value selections.
  $selected = array();
  foreach ($selected_paddocks as $id) {
    $selected[$id] = 1;
  }

  $header = array(
      t('Name'),
      t('Area'),
  );

  $form['checkboxes'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $data,
    '#default_value' => $selected,
    '#empty' => t('No paddocks were found.'),
  );

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Selections'),
    '#submit' => array('farm_grazing_select_paddocks_form_submit'),
  );

  return $form;
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
 * Process the select paddocks form on submit
 */
function farm_grazing_select_paddocks_form_submit($form, &$form_state) {

  $plan = $form_state['values']['plan'];

  // get the old list of selected paddocks for comparison
  $old_selected = get_selected_paddock_for_plan($plan);

  // this will be an array where $key will be the same as $id
  $paddocks = array_filter($form_state['values']['checkboxes']);

  // first add records that have been selected and don't already exist
  foreach ($paddocks as $key => $id) {
    // if the incoming selection exists in the db we have nothing to do
    if (isset($old_selected[$key]) || in_array($key, $old_selected)) {
      continue;
    }
    db_insert('farm_grazing_plan_paddock')
    ->fields(array(
      'paddock_id' => $key,
      'plan_id' => $plan,
    ))
    ->execute();
  }

  // next delete records that exist but are no longer selected
  foreach($old_selected as $key) {
    // if an existing db record is in the incoming selections
    // we have nothing to do
    // otherwise we should delete it
    if (isset($paddocks[$key]) || array_key_exists($key, $paddocks)) {
      continue;
    }
    db_delete('farm_grazing_plan_paddock')
      ->condition('paddock_id', $key)
      ->condition('plan_id', $plan)
      ->execute();
  }
}

/**
 * Paddock exclusions list, add, edit, delete form.
 */
function farm_grazing_exclusions_form($form, &$form_state, $plan_obj) {

  $plan = $plan_obj->id;

  // Set the page title.
  drupal_set_title(t('Paddock exclusions'));

  // Add page instructional information
  $form['text'] = array(
    '#markup' => '<h2>' . t('Paddock Livestock Exclusions and Special Attention') . '</h2>' .
    '<p>' . t('Set dates for paddocks where you want to exclude animal grazing. Check your Planning Factors.') . '</p>',
  );

  // Load a list of exclusions from the database.
  $query = db_query('SELECT e.*, t.name FROM {farm_grazing_paddock_exclusions} e LEFT JOIN taxonomy_term_data t ON e.paddock_id=t.tid ORDER BY t.name, e.start_date');

  // Build a themed table of exclusions.
  $table = array(
    'header' => array(
      t('Paddock'),
      t('Exc. Start'),
      t('Exc. End'),
      t('Reason'),
    ),
    'rows' => array(),
    'empty' => t('No exclusions found.'),
  );
  $records = $query->fetchAll();
  foreach ($records as $record) {

    // If the paddock ID is not available, skip it.
    if (empty($record->paddock_id)) {
      continue;
    }

    // Format the start and end dates.
    $format = 'Y-m-d';
    $start_date = date($format, $record->start_date);
    $end_date = date($format, $record->end_date);

    // Assemble the row.
    $table['rows'][] = array(
      '<a href="/farm/grazing/exclusions/' . $record->id . '/edit?destination=farm/plan/' . $plan . '/paddocks/exclusions">' . $record->name . '</a>',
      $start_date,
      $end_date,
      $record->reason,
    );
  }
  $form['table'] = array(
    '#markup' => theme('table', $table),
  );

  // Load all paddocks and generate an options list.
  $paddocks = farm_area_load_areas('paddock', 'name');
  $paddock_options = array();
  foreach ($paddocks as $paddock) {
    $paddock_options[$paddock->tid] = $paddock->name;
  }

  // Form for adding a new exclusion.
  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add an exclusion'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['paddock_id'] = array(
    '#type' => 'select',
    '#title' => t('Paddock'),
    '#description' => t('Select the paddock to create an exclusion for.'),
    '#options' => $paddock_options,
    '#required' => TRUE,
  );
  $form['add']['start_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start date'),
    '#description' => t('When does the exclusion begin?'),
    '#date_label_position' => 'within',
    '#date_format' => 'M j Y',
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#required' => TRUE,
  );
  $form['add']['end_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('End date'),
    '#description' => t('When does the exclusion end?'),
    '#date_label_position' => 'within',
    '#date_format' => 'M j Y',
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#required' => TRUE,
  );
  $form['add']['reason'] = array(
    '#type' => 'textfield',
    '#title' => t('Reason'),
    '#description' => t('Describe the reason for this exclusion.'),
    '#required' => TRUE,
  );
  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#id' => 'add-submit',
    '#value' => t('Save exclusion'),
    '#submit' => array('farm_grazing_exclusions_form_submit'),
  );

  // Return the form.
  return $form;
}

/**
 * Submit function for the paddock exclusions form.
 */
function farm_grazing_exclusions_form_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state, true));

  // if user clicked Cancel, get out of here
  if ($form_state['clicked_button']['#id'] == 'edit-cancel') {
    return;
  }

  // check if we are doing a create, update or delete
  $id = NULL;
  if (isset($form_state['values']['exclusion_id'])) {
    $id = $form_state['values']['exclusion_id'];
  }

  // Build a record
  $record = array(
    'id' => $id,
    'paddock_id' => $form_state['values']['paddock_id'],
    'start_date' => strtotime($form_state['values']['start_date']),
    'end_date' => strtotime($form_state['values']['end_date']),
    'reason' => $form_state['values']['reason'],
  );

  switch ($form_state['clicked_button']['#id']) {
    case 'add-submit':
      // Add a new record
      drupal_write_record('farm_grazing_paddock_exclusions', $record);
      break;
    case 'edit-submit':
      // Update existing record
      drupal_write_record('farm_grazing_paddock_exclusions', $record, 'id');
      break;
    case 'edit-delete':
      // Delete an existing record
      db_delete('farm_grazing_paddock_exclusions')
        ->condition('id', $id)
        ->execute();
      break;
  }

}

/**
 * Edit/Delete exclusion form callback
 */
function farm_grazing_exclusions_edit_form($form, &$form_state, $exclusion_id) {
  
  // Set page Title
  drupal_set_title(t('Edit or Delete Exclusion'));

  // Load the exclusions from the database.
  $query = db_query('SELECT * FROM {farm_grazing_paddock_exclusions} WHERE id=:id', array(':id' => $exclusion_id));

  // fetch it
  $exclusion = $query->fetchObject();

  // Form for editing an exclusion.
  $form['edit']['exclusion_id'] = array(
    '#type' => 'value',
    '#value' => $exclusion_id,
  );

  $form['edit']['paddock_id'] = array(
    '#type' => 'value',
    '#value' => $exclusion->paddock_id,
  );

  $term = taxonomy_term_load($exclusion->paddock_id);

  // Format the start and end dates.
  $format = 'Y-m-d';
  $format = 'd-m-Y';
  $format = 'M j Y';
  $start_date = date('Y-m-d H:i:s', $exclusion->start_date);
  $end_date = date('Y-m-d H:i:s', $exclusion->end_date);

  $form['edit']['name'] = array(
    '#prefix' => '<h1>',
    '#markup' => $term->name,
    '#suffix' => '</h1>',
  );
  $form['edit']['start_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start date'),
    '#description' => t('When does the exclusion begin?'),
    '#date_label_position' => 'within',
    '#date_format' => $format,
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#default_value' => $start_date,
    '#required' => TRUE,
  );
  $form['edit']['end_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('End date'),
    '#description' => t('When does the exclusion end?'),
    '#date_label_position' => 'within',
    '#date_format' => $format,
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#default_value' => $end_date,
    '#required' => TRUE,
  );
  $form['edit']['reason'] = array(
    '#type' => 'textfield',
    '#title' => t('Reason'),
    '#description' => t('Describe the reason for this exclusion.'),
    '#default_value' => $exclusion->reason,
    '#required' => TRUE,
  );
  $form['edit']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save exclusion'),
    '#submit' => array('farm_grazing_exclusions_form_submit'),
  );
  $form['edit']['cancel'] = array(
    '#type' => 'submit',
    '#value' => t('Cancel'),
    '#submit' => array('farm_grazing_exclusions_form_submit'),
  );
  $form['edit']['delete'] = array(
    '#type' => 'submit',
    '#value' => t('Delete exclusion'),
    '#submit' => array('farm_grazing_exclusions_form_submit'),
  );

  // Return the form.
  return $form;

}

/**
 * Set paddock productivity form callback
 */
function farm_grazing_plan_productivity_form($form, &$form_state, $plan_obj) {
  
  // Set the page title
  drupal_set_title(t('Rate Paddock Productivity'));

  // get the plan id from the plan object
  $plan = $plan_obj->id;

  $form['#tree'] = true;

  // Return markup
  $form['text'] = array(
    '#markup' => t('<p>Enter a value for each paddock in the Forage Quality field, then click the [Save] button. See the Help button for more information.</p>'),
  );

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  $header = array(
    t('Name'),
    t('Forage Quality'),
    t('Est. Relative Quality'),
  );

  // get a list of selected paddocks
  $selected = get_selected_paddock_quality_for_plan($plan);

  $form['paddocks'] = array(
    '#theme' => 'farm_grazing_paddocks_productivity',
    '#header' => $header,
  );

  $num = 0;
  $pad_avg = 0;
  $tot_avg = 0;

  foreach ($selected as $paddock_id => $quality) {
    $term = taxonomy_term_load($paddock_id);
    $form['paddocks'][$paddock_id]['name'] = array(
      '#markup' => $term->name,
    );
    $form['paddocks'][$paddock_id]['quality'] = array(
      '#type' => 'textfield',
      '#default_value' => $quality,
      '#size' => 30,
      '#maxlength' => 30,
    );
    $pad_avg = $pad_avg + $quality;

    $tot_quality = $quality * farm_area_calculate_area($paddock_id);
    $form['paddocks'][$paddock_id]['total'] = array(
      '#prefix' => '<p style="text-align:right">',
      '#markup' => round($tot_quality, 1),
      '#suffix' => '</p>',
    );
    $tot_avg = $tot_avg + $tot_quality;
    $num++;
  }

  $pad_avg = $pad_avg / $num;
  $tot_avg = $tot_avg / $num;

  $form['summary']['avg_rating'] = array(
    '#prefix' => '<p>' . t('Average AD/A or Rating: '),
    '#markup' => round($pad_avg, 1),
    '#suffix' => '</p>',
  );

  $form['summary']['avg_tot_rating'] = array(
    '#prefix' => '<p>' . t('Average Total AD/A or Rating: '),
    '#markup' => round($tot_avg, 1),
    '#suffix' => '</p>',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('farm_grazing_plan_productivity_form_submit'),
    '#validate' => array('farm_grazing_plan_productivity_form_validate'),
  );

  return $form;
}

/**
 * Validate handler for farm_grazing_paddocks_productivity_form
 */
function farm_grazing_plan_productivity_form_validate($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['input'], true));

  foreach ($form_state['input']['paddocks'] as $id => $val) {
    if (! is_numeric($val['quality']) or $val['quality'] < 0.0) {
      form_set_error('quality', t('Forage Quality must be numeric >= 0.0'));
    }
  }
}


/**
 * Process the paddock productivity form on submit
 */
function farm_grazing_plan_productivity_form_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['values'], true));

  $plan = $form_state['values']['plan'];

  foreach ($form_state['values']['paddocks'] as $key => $val) {
    db_update('farm_grazing_plan_paddock')
      ->fields(array(
        'quality' => $val['quality'],
      ))
      ->condition('plan_id', $plan)
      ->condition('paddock_id', $key)
      ->execute();
  }
}

/**
 * Paddock recovery page callback.
 */
function farm_grazing_plan_recovery_form($form, &$form_state, $plan_obj) {

  // Set the page title.
  drupal_set_title(t('Determine recovery periods'));

  // get the plan id from the plan object
  $plan = $plan_obj->id;

  $form['#tree'] = true;

  // Return markup
  $form['text'] = array(
    '#markup' => t('<p>Enter your minimum and maximum recovery periods (in days) for each planned month.</p>'),
  );

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  $header = array(
    t('Month'),
    t('Min. Recovery Days'),
    t('Max. Recovery Days'),
  );

  $minmax = getPlanRecovery($plan);

  // get the start date month and year
  $start_date = new DateTime();
  $start_date->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value']);

  // get the end date month and year
  $end_date = new DateTime();
  $end_date->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value2']);

  // get the DateInterval between them
  $diff = $start_date->diff($end_date);

  $form['months'] = array(
    '#theme' => 'farm_grazing_paddocks_recovery',
    '#header' => $header,
  );

  // generate array of months between start and end
  $num_months = $diff->y * 12 + $diff->m;
  $date = $start_date;
  $minmin =  999999999;
  $maxmax = -999999999;
  $num_paddocks = 0;
  $rows = array();
  for( $i = 0; $i <= $num_months; $i++ ) {
    $key = $date->format('Ym');
    if (isset($minmax[$key])) {
      $min = $minmax[$key][0];
      $max = $minmax[$key][1];
      $num_paddocks++;
    }
    else {
      $min = 0;
      $max = 0;
    }
    if ($min > 0 and $min < $minmin) {
      $minmin = $min;
    }
    if ($max > 0 and $maxmax < $max) {
      $maxmax = $max;
    }
    $form['months'][$key]['month'] = array(
      '#markup' => $date->format('F Y'),
    );
    $form['months'][$key]['min'] = array(
      '#type' => 'textfield',
      '#default_value' => $min,
      '#size' => 30,
      '#maxlength' => 30,
    );
    $form['months'][$key]['max'] = array(
      '#type' => 'textfield',
      '#default_value' => $max,
      '#size' => 30,
      '#maxlength' => 30,
    );

    // increment the month
    $date->modify('+1 month');
  }

  // get herd count from database or 1.0
  $herd_cnt = getHerdCountForPlan($plan);
  if ($herd_cnt == 0) {
    $herd_cnt = 1;
  }

  $form['summary']['herd_cnt'] = array(
    '#prefix' => '<p>' . t('Number of herds: '),
    '#suffix' => t(' (Defaults to one herd if none are defined)') . '</p>',
    '#markup' => $herd_cnt,
  );

  $form['summary']['pad_cnt'] = array(
    '#prefix' => '<p>' . t('Number of paddocks: '),
    '#markup' => $num_paddocks,
    '#suffix' => '</p>',
  );
  $form['summary']['min_grazing'] = array(
    '#prefix' => '<p>' . t('Est. Min. Grazing Period: '),
    '#markup' => round($minmin/($num_paddocks-$herd_cnt)),
    '#suffix' => '</p>',
  );
  $form['summary']['max_grazing'] = array(
    '#prefix' => '<p>' . t('Est. Max. Grazing Period: '),
    '#markup' => round($maxmax/($num_paddocks-$herd_cnt)),
    '#suffix' => '</p>',
  );


  // Return markup.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('farm_grazing_plan_recovery_form_submit'),
    '#validate' => array('farm_grazing_plan_recovery_form_validate'),
  );

  return $form;
}

/**
 * getPlanRecovery($plan)
 */
function getPlanRecovery($plan) {
  $minmax = array();
  $results = db_query('select month, min_recovery, max_recovery from {farm_grazing_plan_recovery} where plan_id=:plan', array(':plan' => $plan));
  foreach($results as $record) {
    $minmax[$record->month] = array($record->min_recovery, $record->max_recovery);
  }
  return $minmax;
  
}


/**
 * Validate handler for farm_grazing_paddocks_recovery_form
 */
function farm_grazing_plan_recovery_form_validate($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['input'], true));

  foreach ($form_state['input']['months'] as $id => $val) {
    if (! is_numeric($val['min']) or $val['min'] < 0.0) {
      form_set_error('min_recovery', t('Min. Recovery Days must be numeric >= 0'));
    }
    if (! is_numeric($val['max']) or $val['max'] < 0.0) {
      form_set_error('max_recovery', t('Max. Recovery Days must be numeric >= 0'));
    }
    if ($val['min'] > $val['max']) {
      form_set_error('max_recovery', t('Max. Recovery Days must be greater than Min. Recovery Days'));
    }
  }
}


/**
 * Process the paddock recovery form on submit
 */
function farm_grazing_plan_recovery_form_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['values'], true));

  $plan = $form_state['values']['plan'];

  foreach ($form_state['values']['months'] as $key => $val) {
    db_merge('farm_grazing_plan_recovery')
      ->key(array(
        'plan_id' => $plan,
        'month' => $key,
      ))
      ->fields(array(
        'min_recovery' => $val['min'],
        'max_recovery' => $val['max'],
      ))
      ->execute();
  }
}

/**
 * Paddock summary page callback
 */
function farm_grazing_plan_summary_page($plan_obj) {
  
  // get the plan id from the plan object
  $plan = $plan_obj->id;

  // Set the page title
  drupal_set_title(t('Paddock Summary and Grazing Periods'));

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

  // Return markup
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

  return $form;
}

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
 * Paddocks page callback.
 */
function farm_grazing_plan_herds_paddocks_form($form, &$form_state, $plan_obj) {

  $plan = $plan_obj->id;

  // Set the page title.
  drupal_set_title(t('Select paddocks for a herd'));

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );
  $form['text'] = array(
    '#markup' => t('
      <p>Add or remove paddocks you want to use for each herd. Paddocks may be added more than once for a given herd or between multiple herds.</p>'),
  );

  // Get a list of paddocks to build options array
  $query = db_query('SELECT p.*, t.name FROM {farm_grazing_plan_paddock} p LEFT JOIN taxonomy_term_data t ON p.paddock_id=t.tid ORDER BY t.name');
  $records = $query->fetchAll();

  // build the select options array
  $paddock_options = array();
  foreach ($records as $rec) {
    $paddock_options[$rec->paddock_id] = $rec->name;
  }

  // build sections for each herds associated with this plan
  $query = db_query('SELECT h.*, a.name FROM {farm_grazing_herds} h JOIN {farm_asset} a ON h.herd_id=a.id WHERE plan_id=:plan_id ORDER BY a.name',
    array(':plan_id' => $plan));
  $records = $query->fetchAll();

  $herd_options = array();
  foreach ($records as $rec) {
    $herd_options[$rec->herd_id] = $rec->name;
  }

  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add a paddock to herd'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['herd_id'] = array(
    '#type' => 'select',
    '#title' => t('Herd'),
    '#description' => t('Select a herd you want to add a paddock to.'),
    '#options' => $herd_options,
    '#required' => TRUE,
  );
  $form['add']['paddock_id'] = array(
    '#type' => 'select',
    '#title' => t('Paddock'),
    '#description' => t('Select a paddock you want to add to this herd.'),
    '#options' => $paddock_options,
    '#required' => TRUE,
  );
  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add paddock to herd'),
    '#submit' => array('farm_grazing_plan_herds_paddocks_form_submit'),
  );

  // foreach herd
  reset($records);
  foreach ($records as $rec) {

    // create a section for the herd and list the paddocks assigned to it
    $form['section_title_' . $rec->herd_id] = array(
      '#markup' => '<h4>' . t('Paddocks for ') . $rec->name . '</h4>',
    );

    // build a table of paddocks assigned to this herd
    //$query = db_query('SELECT * FROM {farm_grazing_herd_paddocks} WHERE plan_id=:plan_id AND herd_id=:herd_id',
    $query = db_query('SELECT * FROM {farm_grazing_rotations} WHERE plan_id=:plan_id AND herd_id=:herd_id',
      array(':plan_id' => $plan, ':herd_id' => $rec->herd_id));
    $records = $query->fetchAll();

    $rows = array();
    foreach ($records as $rec) {
      $term = taxonomy_term_load($rec->paddock_id);
      $area = farm_area_calculate_area($rec->paddock_id);
      $rows[] = array(
        $term->name,
        $area,
        '<a href="/farm/plan/' . $plan . '/herds/' . $rec->herd_id .
          '/paddocks/' . $rec->paddock_id . '/delete/' . $rec->id .
          '?destination=farm/plan/' . $plan .  '/herds/paddocks">' .
          t('Remove') . '</a>',
      );
    }

    $table = array(
      'header' => array(
        t('Paddock'),
        t('Area'),
        t('Remove'),
      ),
      'rows' => $rows,
      'empty' => t('No paddocks have been added to this herd.'),
    );

    $form['section_table_' . $rec->herd_id] = array(
      '#markup' => theme('table', $table),
    );

  } // end foreach herd

  return $form;
}

/**
 *
 */
function farm_grazing_plan_herds_paddocks_form_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state, true));
  //fputs($fp, print_r($form_state['values'], true));

  $plan_id = $form_state['values']['plan'];
  $herd_id = $form_state['values']['herd_id'];
  $paddock_id = $form_state['values']['paddock_id'];

  //db_insert('farm_grazing_herd_paddocks')
  db_insert('farm_grazing_rotations')
    ->fields(array(
      'paddock_id' => $paddock_id,
      'plan_id' => $plan_id,
      'herd_id' => $herd_id,
      ))
      ->execute();

}

/**
 *
 */
function farm_grazing_plan_herds_paddocks_delete($form, &$form_state, $plan, $herd_id, $paddock_id, $rid) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state, true));
  //fputs($fp, print_r(array('plan_id'=>$plan, 'rid'=>$rid)));

  $form['plan_id'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );
  $form['rid'] = array(
    '#type' => 'value',
    '#value' => $rid,
  );

  $group = farm_asset_load($herd_id);
  $term = taxonomy_term_load($paddock_id);

  return confirm_form($form,
    t('Are you sure you want to remove paddock ' . $term->name . ' from ') . $group->name . t('?'),
    drupal_get_destination(),
    t('You can add it back later if you change your mind.'),
    t('Remove it!'),
    t('Cancel')
    );
}

function farm_grazing_plan_herds_paddocks_delete_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state, true));

  //db_delete('farm_grazing_herd_paddocks')
  db_delete('farm_grazing_rotations')
    ->condition('id', $form_state['values']['rid'])
    ->execute();

}

/**
 * History page callback.
 */
function farm_grazing_plan_history_form($form, $form_state, $plan_obj=null) {

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
    //'#submit' => array('farm_grazing_plan_history_form'),
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
      dpm($paddock_ids);
      $paddocks = taxonomy_term_load_multiple($paddock_ids);
    }

    // Iterate through the paddocks.
    foreach ($paddocks as $paddock) {

      // Query for the paddock's history of Group assets, going back to the
      // requested timestamp.
      $history = farm_movement_area_history($paddock, 'group', $start_time);

      // Iterate through the asset histories.
      foreach ($history as $asset_id => $movements) {

        // Check to make sure that the group has animal assets in it.
        /**
         * @todo
         * This will need to be added when Group membership is done.
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
          //...
        }
      }

      // TODO add data to table rows
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

/**
 * Periods page callback.
 */
function farm_grazing_plan_rotations_form($form, &$form_state, $plan_obj) {

  $plan = $plan_obj->id;

  // Set the page title.
  drupal_set_title(t('Select paddock rotation order for each herd'));

  $form['#tree'] = TRUE;

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );
  $form['text'] = array(
    '#markup' => t('
      <p>Drag and drop to reorder the paddocks for each herd. Avoid grazing during paddock exclusions or recovery periods.</p>'),
  );

  // build optional control to reset the number of grazing dats
  $form['reset_grazing_days'] = array(
    '#type' => 'fieldset',
    '#title' => t('Reset Grazing Days'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['reset_grazing_days']['select'] = array(
    '#type' => 'select',
    '#title' => t('Which Grazing Days'),
    '#description' => t('Select the option to reset all fields below.'),
    '#options' => array(
      '0' => '- ' . t('Select Grazing Days Option') . ' -',
      '1' => 'Minimum grazing days',
      '2' => 'Average grazing days',
      '3' => 'Maximum grazing days',
    ),
    '#required' => FALSE,
  );
  $form['reset_grazing_days']['submit'] = array(
    '#type' => 'submit',
    '#id' => 'reset-submit',
    '#value' => t('Update Grazing Days'),
    '#submit' => array('farm_grazing_plan_rotations_form_submit'),
  );

  // build sections for each herds associated with this plan
  $query = db_query('SELECT h.*, a.name FROM {farm_grazing_herds} h JOIN {farm_asset} a ON h.herd_id=a.id WHERE plan_id=:plan_id ORDER BY a.name',
    array(':plan_id' => $plan));
  $records = $query->fetchAll();

  $form['herds'] = array();

  $cache = NULL;

  // foreach herd
  foreach ($records as $rec) {

    // create a section for the herd and list the paddocks assigned to it
    $form['herds'][$rec->herd_id]['title'] = array(
      '#markup' => '<h4>' . t('Rotations for herd ') . $rec->name . '</h4>',
    );

    // build a table of paddocks assigned to this herd
    $query = db_query('SELECT p.id, p.plan_id, p.herd_id, p.paddock_id,
        p.duration, p.start_date, coalesce(p.weight, 0) as weight, t.name
      FROM {farm_grazing_rotations} p
      LEFT JOIN taxonomy_term_data t ON p.paddock_id=t.tid
      WHERE plan_id=:plan_id AND herd_id=:herd_id
      ORDER BY 7, 6 ASC',
      array(':plan_id' => $plan, ':herd_id' => $rec->herd_id));
    $paddocks = $query->fetchAll();

    $rows = array();
    $arrival = getHerdEarliestArrival($rec->herd_id);
    $start_date = date_create_from_format('Y-m-d', $arrival);
    $i = 1;
    foreach ($paddocks as $pad) {
      // weight might be null
      $i = max($pad->weight, $i);
      $grazing_days = getPaddockGrazingDays($pad->duration, $pad->paddock_id, $plan, $rec->herd_id, $minOrMax);
      $start_str = $start_date->format('Y-m-d');
      $end_date = $start_date;
      $end_date->modify('+' . round($grazing_days) . ' days');
      $end_str = $end_date->format('Y-m-d');
      $form['herds'][$rec->herd_id]['rotations'][$pad->id] = array(
        'name' => array(
          '#markup' => $pad->name,
        ),
        'grazing_days' => array(
          '#type' => 'textfield',
          '#default_value' => round($grazing_days,1),
          '#size' => 10,
          '#maxlength' => 10,
        ),
        'start_date' => array(
          '#type' => 'textfield',
          '#default_value' => $start_str,
          '#disabled' => TRUE,
        ),
        'end_date' => array(
          '#type' => 'textfield',
          '#default_value' => $end_str,
          '#disabled' => TRUE,
        ),
        'image' => array(
          '#markup' => '<img src="/farm/grazing/plan/' .  $plan .
            '/plot/' . $pad->paddock_id . '/' . $start_str . '/' . $end_str .
            '">',
        ),
        'weight' => array(
          '#type' => 'weight',
          '#title' => t('Order'),
          '#default_value' => $i,
          '#delta' => 10,
          '#title_display' => 'invisible',
          '#attributes' => array(
            'class' => array('herd_' . $rec->herd_id . '_weight'),
          ),
        ),
      );
      $start_date = $end_date;
      $i++;
    }

  } // end foreach herd

  // Now we add our submit button, for submitting the form results.
  //
  // The 'actions' wrapper used here isn't strictly necessary for tabledrag,
  // but is included as a Form API recommended practice.
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save Changes'));

  //kpr($form);

  // Return markup.
  return $form;
}

function theme_farm_grazing_plan_rotations_form($variables) {
  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($variables, true));

  $form =& $variables['form'];

  $plan = $form['plan']['#value'];

  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['text']);

  $output .= drupal_render($form['reset_grazing_days']);

  foreach (element_children($form['herds']) as $herd_id) {
    // render the tables title
    $output .= drupal_render($form['herds'][$herd_id]['title']);

    // assemble the table rows
    $rows = array();
    foreach (element_children($form['herds'][$herd_id]['rotations']) as $rid) {
      $item =& $form['herds'][$herd_id]['rotations'][$rid];
      $rows[] = array(
        'data' => array(
          drupal_render($item['name']),
          drupal_render($item['grazing_days']),
          drupal_render($item['start_date']),
          drupal_render($item['end_date']),
          drupal_render($item['image']),
          drupal_render($item['weight']),
        ),
        'class' => array('draggable'),
      );
    }

    $header = array(
      t('Paddock'),
      t('Grazing Days'),
      t('Start Date'),
      t('End Date'),
      '<img src="/farm/grazing/plan/'.$plan.'/plot/0/0/0">',
      t('Sort weight'),
    );

    $table_id = 'herd_' . $herd_id . '_rotations';

    // render the table into the the output
    $output .= theme('table', array(
      'header' => $header,
      'rows' => $rows,
      'attributes' => array('id' => $table_id),
    ));

    // setup the table for drag and drop
    $weight_class = 'herd_' . $herd_id . '_weight';
    drupal_add_tabledrag($table_id, 'order', 'sibling', $weight_class);
  }

  // render any remaining elements
  $output .= drupal_render_children($form);

  return $output;
}

/**
 *
 */
function farm_grazing_plan_rotations_form_submit($form, &$form_state) {
  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['values'], true));

  $plan_id = $form_state['values']['plan'];

  if ($form_state['values']['op'] == t('Save Changes')) {
    foreach ($form_state['values']['herds'] as $herd_id => $rotations) {
      foreach ($rotations['rotations'] as $rid => $data) {
        db_update('farm_grazing_rotations')
          ->fields(array(
            'duration' => $data['grazing_days'],
            'weight' => $data['weight'],
            'start_date' => strtotime($data['start_date']),
          ))
          ->condition('id', $rid)
          ->condition('herd_id', $herd_id)
          ->condition('plan_id', $plan_id)
          ->execute();
      }
    }
  }
  elseif ($form_state['values']['op'] == t('Update Grazing Days')) {
    // if Update Grazing Days was click but select is 0
    // we have nothing to update so return
    $mode = $form_state['values']['reset_grazing_days']['select'];
    if ($mode == 0) {
      return;
    }
    // build data as we are going to need it
    $paddocks = getGrazingDaysForPlanId($plan_id, $d1, $d2, true);
    $data = array();
    foreach ($paddocks as $p) {
      $data[$p[5]] = $p;
    }

    // update all rotation records in plan
    $records = db_query('SELECT id, paddock_id, herd_id FROM {farm_grazing_rotations} WHERE plan_id=:plan_id', array(':plan_id' => $plan_id));

    $cache = array();
    foreach ($records as $rec) {
      $days = getPaddockGrazingDays(NULL, $rec->paddock_id, $plan_id, $rec->herd_id, $cache, $mode);
      db_update('farm_grazing_rotations')
        ->fields(array(
          'duration' => $days,
          'start_date' => NULL,
          ))
        ->condition('id', $rec->id)
        ->execute();
    }

  }

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
 * Implement page callback.
 */
function farm_grazing_plan_implement_page($plan) {

  // Set the page title.
  drupal_set_title(t('Implement the plan'));

  // Return markup.
  return array(
    '#markup' => t('
      <h2>Recheck your plan</h2>
      <p>Review your plan regularly, print out reports, check that the plan is on track and make adjustments as needed.</p>
      <h2>Monitor Your Paddocks</h2>
      <p>Monitor the growth and recovery of your paddocks. Also assess the quality of the paddock you are moving the herd to and record it as a monitor event here.</p>
      <h2>Record the Actual Moves</h2>
      <p>When you make an actual move assess the residual feed left on the paddock and enter it into the actual move by clicking on the move event in the calendar. Note what kind of growth rate you had while occupying the paddock. If there was a serious error made in the paddock you are leaving check the box and add a description performance area.</p>
      <p>Check the condition of the paddock that you are moving the herd into and add a monitor event using the button above.</p>
      <h2>How to Replan if Needed</h2>
      <p>You can make changes to your plan at any time. Actual events obviously can not be changed, but you can replan future events. There are two approaches to replanning. 1) is to just update the current plan for minor changes, 2) another is to create a new plan, for major changes, based on the remaining days and abandon the old plan for now. The advantage to making a new plan is that you can keep the original plan around for archival purposes and look at it in the future to re-assess assumptions or conditions that changed and forced the replan. On the new plan, you might need to sell stock to reduce the requirements for intake if you are in a drought or had a fire that burnt through some padocks or add more paddocks that you had not planned to use in the original plan. If you only need to make minor changes then use your existing plan, otherwise start a new plan.</p>'),
  );
}

/**
 *
 */
function farm_grazing_plan_implement_status_page($plan) {

  // Set the page title
  drupal_set_title(t('Plan Status'));

  // Return markup
  return array(
    '#markup' => t('
      <p><b>TODO:</b> generate plan status report.</p>
    '),
  );
}

// -------------- config forms ---------------------

/**
 * function farm_grazing_config_animal_types_form
 *
 * TODO I think Mike is doing this elsewhere??
 */
function farm_grazing_config_animal_types_form($form, $form_state) {

}


// -------------- utility functions ----------------

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
 * TODO Remove dummy function farm_group_get_members
 */
function farm_group_get_members($obj) {
  return array();
}

/**
 * get_paddock_exclusions($plan_id, $paddock_id)
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
 * @return array where sday and eday are the number of days from start of plan
 *   for the exclusion start date and end date or zero if before plan start
 */
function get_paddock_exclusions($plan_id, $paddock_id) {
  // load the plan
  list($p_start, $p_end) = getFarmPlanDatesFromId($plan_id, 'Y-m-d');

  // get query paddock exclusions
  $records = db_query('SELECT * FROM {farm_grazing_paddock_exclusions} WHERE paddock_id=:paddock_id', array(':paddock_id' => $paddock_id));

  $results = array();
  foreach ($records as $record) {
    // get the exclusion start_date as YYYY-MM-DD
    $e_start = new DateTime();
    $e_start->setTimestamp($record->start_date);
    $e_start = $e_start->format('Y-m-d');

    // get the exclusion end_date as YYYY-MM-DD
    $e_end = new DateTime();
    $e_end->setTimestamp($record->end_date);
    $e_end = $e_end->format('Y-m-d');

    // exclusion start_date as days from start of plan
    $sday = get_days_between_dates($p_start, $e_start);

    // exclusion start_date as days from start of plan
    $eday = get_days_between_dates($p_start, $e_end);

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
 * get_paddock_recovery($plan_id, $paddock_id)
 *
 *   $recovery = array(
 *     array(
 *       'sday' => (rec_start - plan_start) as days,
 *       'eday' => (rec_end   - plan_start) as days,
 *     ),
 *     ...
 *   );
 */
function get_paddock_recovery($plan_id, $paddock_id) {
  // TODO query and return results
  return array();
}

/**
 * get_social_events($plan_id)
 *
 *   $social = array(
 *     array(
 *       'sday' => (soc_start - plan_start) as days,
 *       'eday' => (soc_end   - plan_start) as days,
 *     ),
 *     ...
 *   );
 */
function get_social_events($plan_id) {
  // TODO query and return results
  return array();
}

/**
 * Generate an image that plots a paddock rotation visualization.
 *
 * Parameters from URL
 * @param plan_obj   - drupal plan object
 * @param paddock_id - int, paddock id for this plan
 * @param start_date - string YYYY-MM-DD, start of paddock grazing
 * @param end_date   - string YYYY-MM-DD, end of paddock grazing
 * @returns an image showing rotation in calendar or calendar header image
 */
function farm_grazing_plan_plot($plan_obj, $paddock_id, $start_date, $end_date) {
  // Set the header Content-type to 'image/png'.
  drupal_add_http_header('Content-Type', 'image/png');

  $plan = $plan_obj->id;
  $header = 0;

  // if paddock_id, start_date and end_date == 0 then generate the header image
  // If any of the arguments are empty, bail with a 404 not found.
  if (isset($plan) &&
      isset($paddock_id) && $paddock_id==0 &&
      isset($start_date) && $start_date==0 &&
      isset($end_date)   && $end_date==0) {
    // generate header image
    $header = 1;
  }
  elseif (empty($plan) || empty($paddock_id) || empty($start_date) || empty($end_date)) {
    drupal_not_found();
    drupal_exit();
  }

  // set image width and height
  $width =  550;
  $height =  16;

  // get the start date month and year
  $plan_start = new DateTime();
  $plan_start->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value']);
  $plan_start = $plan_start->format('Y-m-d');

  // get the end date month and year
  $plan_end = new DateTime();
  $plan_end->setTimestamp($plan_obj->field_farm_date_range['und'][0]['value2']);
  $plan_end = $plan_end->format('Y-m-d');

  // get number of days in plan
  $plan_days = get_days_between_dates($plan_start, $plan_end);

  // get number of days from start of plan to start_date
  $days_to_start = get_days_between_dates($plan_start, $start_date);

  // get number of days from start of plan to end_date
  $days_to_end = get_days_between_dates($plan_start, $end_date);

  // get day width in pixels and adjust image width
  $day_width = round($width / ($plan_days + 1));
  $width = $plan_days * $day_width;

  if (! $header) {
    $exclusions = get_paddock_exclusions($plan, $paddock_id);
    $recovery = get_paddock_recovery($plan, $paddock_id);
    $social = get_social_events($plan);
  }

  // Create a demo image.
  $im = @imagecreate($width, $height)
  or die("Cannot Initialize new GD image stream");

  $white = imagecolorallocate($im, 255, 255, 255);
  imagefill($im, 0, 0, $white);

  if (! $header) {
    // add social events
    $purple = imagecolorallocate($im, 255, 0, 255);
    foreach ($social as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 0;
      $x2 = $event['eday'] * $day_width;
      $y2 = $height;
      imagefilledrectangle($im, $x1, $y1, $x2, $y2, $purple);
    }

    // add exclusion periods
    $red = imagecolorallocate($im, 255, 0, 0);
    foreach ($exclusions as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 3;
      $x2 = $event['eday'] * $day_width;
      $y2 = $height-3;
      imagefilledrectangle($im, $x1, $y1, $x2, $y2, $red);
    }

    // add recovery periods
    $orange = imagecolorallocate($im, 255, 128, 0);
    foreach ($recovery as $event) {
      $x1 = $event['sday'] * $day_width;
      $y1 = 5;
      $x2 = $event['eday'] * $day_width;
      $y2 = $height-5;
      imagefilledrectangle($im, $x1, $y1, $x2, $y2, $orange);
    }

    // add the paddock for grazing period
    $green = imagecolorallocate($im, 0, 255, 0);
    $x1 = $days_to_start * $day_width;
    $y1 = 6;
    $x2 = $days_to_end * $day_width;
    $y2 = $height-6;
    imagefilledrectangle($im, $x1, $y1, $x2, $y2, $green);
  }

  $black = imagecolorallocate($im, 0, 0, 0);

  // draw the month boundaries
  $months = array(t('Jan'), t('Feb'), t('Mar'), t('Apr'), t('May'), t('Jun'),
    t('Jul'), t('Aug'), t('Sep'), t('Oct'), t('Nov'), t('Dec'));

  $days = daysAvailableByMonth($plan_start, $plan_end, $plan_start, $plan_end);

  list($y, $m, $d) = explode('-', $plan_start);
  $sum = 0;
  foreach ($days as $num) {
    if ($num == 0) continue;
    $sum += $num;
    $x1 = $sum * $day_width;
    $y1 = 0;
    $x2 = $x1;
    $y2 = $height;
    imageline($im,  $x1, $y1, $x2, $y2, $black);
    if ($header) {
      imagestring($im, 2, $x1+10, $y1+3, $months[$m%12], $black);
    }
    $m++;
  }

  // output the image, 2nd param is filename, NULL sends to stdout
  //imagepng($im, '/var/www/html/junk.png', 0, NULL);
  imagepng($im);
  imagedestroy($im);

  drupal_exit();
}

