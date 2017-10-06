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
  //'page callback' => 'farm_grazing_plan_herds_paddocks_page',
  //'page callback' => 'farm_grazing_plan_history_page',
  //'page callback' => 'farm_grazing_plan_rotations_page',
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
  $selected = get_selected_paddock_for_plan($plan);

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
 */
function get_selected_paddock_for_plan($plan) {
  // query table of paddocks for this plan 
  // and build an array of those that are currently selected
  $selected = array();
  $results = db_query('select paddock_id from {farm_grazing_plan_paddock} where plan_id=:plan', array(':plan' => $plan));
  foreach($results as $record) {
    $selected[$record->paddock_id] = 1;
  }
  return $selected;
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
    if (isset($old_selected[$key]) || array_key_exists($key, $old_selected)) {
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
  foreach($old_selected as $key => $val) {
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
      '<a href="/farm/grazing/exclusions/' . $record->id . '/edit?destination=/farm/plan/' . $plan . '/paddocks/exclusions">' . $record->name . '</a>',
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
    $paddocks[] = array(
      $term->name,
      round($area, 1),
      round($area*$quality, 1),
      $area*$quality*$avg_min_graz,
      $area*$quality*$avg_max_graz,
    );
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
  $paddocks[] = array(
    t('Sum of Paddocks'),
    round($sum_area, 1),
    round($sum_quality, 1),
    $sum_min,
    $sum_max,
  );

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
    '#markup' => t('Check Min. Recovery Period: ') . round($sum_min - $max_min),
    '#suffix' => '</p>',
  );

  $form['summary']['check_max'] = array(
    '#prefix' => '<p>',
    '#markup' => t('Check Max. Recovery Period: ') . round($sum_max - $max_max),
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
      t('A herd is defined as a group of animals that will be rotated between paddocks to together regardless of the make up of animals in the herd.') .
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
  );

  $table = array(
    'header' => array(
      t('Herd Name'),
      t('SAU'),
      t('Daily Intake'),
      t('Earliest Arrival'),
      t('Latest Departure'),
    ),
    'rows' => array(),
    'empty' => t('No herds found in plan'),
  );

  $query = db_query('SELECT * FROM {farm_grazing_herds} WHERE plan_id=:plan_id', array(':plan_id' => $plan));

  $records = $query->fetchAll();
  foreach ($records as $record) {

    if (empty($record->herd_id)) {
      continue;
    }

    // TODO get group attributes for herd_id
    //  herd name as $herd_name
    //  Total SAU as $herd_sau
    //  Total DMI as $herd_dmi
    //  min(arrival) as $arrival
    //  max(departure) as $departure
    $herd = farm_grazing_herd_summary_info($record->herd_id);

    //$format = 'Y-m-d';
    //$arrival_date = date($format, $herd['arrival']);
    //$departure_date = date($format, $herd['departure']);
    $arrival_date = $herd['arrival'];
    $departure_date = $herd['departure'];

    $table['rows'][] = array(
      '<a href="/farm/grazing/plan/' . $plan . '/herd/' . $record->herd_id .
        '/edit?destination=/farm/plan/' . $plan . '/herds">' .
        $herd['name'] . '</a>',
      $herd['herd_sau'],
      $herd['herd_dmi'],
      $arrival_date,
      $departure_date,
    );
  }

  $form['section_1'] = array(
    '#markup' => '<h4>' . t('Summary of Herds in Plan') . '</h4>',
  );
  // TODO this does not display the table for some reason
  //$form['section_1']['table'] = array(
  $form['section_1_table'] = array(
    '#markup' => theme('table', $table),
  );

  // TODO Add table of herd details for each herd
  $form['section_2'] = array(
    '#markup' => '<h4>' . t('Herd Details') . '</h4>',
  );
  $form['section_2_table'] = array(
    '#markup' => 'TODO',
  );

  // TODO Add table SAU by Plan Month for each herd
  $form['section_3'] = array(
    '#markup' => '<h4>' . t('SAU by Month') . '</h4>',
  );
  $form['section_3_table'] = array(
    '#markup' => 'TODO',
  );
  // TODO Add table SAU by Plan Month for each herd
  $form['section_4'] = array(
    '#markup' => '<h4>' . t('Summary') . '</h4>',
  );
  $form['section_4_table'] = array(
    '#markup' => 'TODO',
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
  $herd_id = $form_state['values']['herd_id'];

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
function farm_grazing_plan_herds_paddocks_page($plan) {

  // Set the page title.
  drupal_set_title(t('Select paddocks for a herd'));

  // Return markup.
  return array(
    '#markup' => t('
      <p>Select a herd if you have defined more than one</p>
      <p>Then select or unselect the paddocks you want to use for this herd. Paddocks may be selected more than once for a given herd or between multiple herds. Once a paddock has be selected for grazing it will show the recovery peroid for that paddock and it is not recommended that it be grazed again until after the recovery period.</p>'),
  );
}

/**
 * History page callback.
 */
function farm_grazing_plan_history_page($plan) {

  // Set the page title.
  drupal_set_title(t('Check historical grazing patterns'));

  // Return markup.
  return array(
    '#markup' => t('
      <p> Skip this step if this is your first grazing plan. Otherwise, check paddock by paddock over previous grazing charts for evidence of inappropriate heavy use of individual paddocks, repeated early or late season use of particular paddocks, or of paddocks that failed to receive adequate recovery time in the recent past. If paddocks were marked as heavily grazed, especially early or late in the season last year, exclude them in the early and late growing season this year to avoid repetition.</p>
      <p>This table counts the frequency of historical use in the given months. Ideally one would like to have these numbers consistent across the months in the plan over the long term. In the short term avoid reuse at start or end of the growing season. Also if a paddock has been used a lot in a given month then avoid using that month also.</p>'),
  );
}

/**
 * Periods page callback.
 */
function farm_grazing_plan_rotations_page($plan) {

  // Set the page title.
  drupal_set_title(t('Select paddock rotation order for each herd'));

  // Return markup.
  return array(
    '#markup' => '
      <p><b>TODO</b></p>
    '
  );
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
 * farm_grazing_herd_summary_info($herd_id)
 *
 * recursively walks a farmos group structure and returns a summary
 * as array(id=>..., name=>...,herd_sau=>...,herd_dmi=>...,
 *          arrival=>...,departure=>...)
 */
function farm_grazing_herd_summary_info($herd_id) {

  $herd = farm_asset_load($herd_id);

  // a herd MUST be a farmos group asset
  if ($herd->type != 'group') {
    return;
  }

  // TODO this function needs to be converted to recursively walk
  //      the subgroups of this herd and summarize all this information

  // get the group quantity for head count
  $head_count = 100; //TODO what function to get quantity of group

  // get the dry matter intake factor as percent of weight
  $animal_type = farm_asset_property_get($herd_id, 'farm_grazing_animal_type');
  $dmi_factor = db_query('SELECT dmi_factor FROM farm_grazing_animal_types WHERE type_id=:tid', array(':tid' => $animal_type))->fetchField();

  // get the average animal weight from the logs
  $weight = 300; // TODO what function to get weight?

  // get the planned arrival anf departure for the herd
  $arrival = farm_asset_property_get($herd_id, 'farm_grazing_planned_arrival');
  $departure = farm_asset_property_get($herd_id, 'farm_grazing_planned_departure');

  return array(
    'id' => $herd_id,
    'name' => $herd->name,
    'herd_sau' => $head_count * $weight / 1000.0,
    'herd_dmi' => $head_count * $weight * $dmi_factor / 100.0,
    'arrival' => $arrival,
    'departure' => $departure,
  );
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
  // TODO farm_grazing_validate_herd($herd_id)

  // Things to validate
  //  farm_grazing_animal_type property exists and is valid
  //  farm_grazing_planned_arrival property exists and is valid
  //  farm_grazing_planned_departure property exists and is valid
  //  has weight
  //  all animals/groups are in some paddock

}


