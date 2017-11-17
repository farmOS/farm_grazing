<?php

include_once "farm_grazing.utils.inc";

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
      l($record->name, 'farm/grazing/exclusions/' . $record->id . '/edit', array('query' => array('destination' => 'farm/plan/' . $plan . '/paddocks/exclusions'))),
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

