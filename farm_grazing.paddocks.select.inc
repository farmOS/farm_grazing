<?php

/**
 * @file
 * Grazing paddock select form.
 */

/**
 * Generate form with table of paddocks callback.
 */
function farm_grazing_select_paddocks_form($form, &$form_state, $plan_obj) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

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
      l($paddock->name, url('taxonomy/term/' . $paddock->tid), array('query' => array('destination' => current_path()))),
      farm_area_calculate_area($paddock->tid),
    );
  }

  // get a list of selected paddocks
  $selected_paddocks = farm_grazing_get_selected_paddock_for_plan($plan);

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
 * Process the select paddocks form on submit
 */
function farm_grazing_select_paddocks_form_submit($form, &$form_state) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  $plan = $form_state['values']['plan'];

  // get the old list of selected paddocks for comparison
  $old_selected = farm_grazing_get_selected_paddock_for_plan($plan);

  // this will be an array where $key will be the same as $id
  $paddocks = array_filter($form_state['values']['checkboxes']);

  // first add records that have been selected and don't already exist
  foreach ($paddocks as $key => $id) {
    // if the incoming selection exists in the db we have nothing to do
    if (in_array($key, $old_selected)) {
      continue;
    }
    db_insert('farm_grazing_plan_paddock')
      ->fields(array(
        'paddock_id' => $key,
        'plan_id' => $plan,
      ))
      ->execute();
  }

  // Next, delete paddocks that are no longer selected. Remember if paddocks
  // are removed so we can alert the user afterwards.
  $paddocks_removed = FALSE;
  foreach ($old_selected as $key) {
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

    // Look up all rotations that reference this plan and paddock, and delete logs.
    $result = db_query('SELECT log_id FROM {farm_grazing_rotations} WHERE plan_id = :plan_id AND paddock_id = :paddock_id', array('plan_id' => $plan, ':paddock_id' => $key));
    foreach ($result as $row) {
      if (!empty($row->log_id)) {
        entity_delete('log', $row->log_id);
      }
    }

    // Delete all rotations that reference this plan and paddock.
    db_delete('farm_grazing_rotations')
      ->condition('plan_id', $plan)
      ->condition('paddock_id', $key)
      ->execute();

    // Remember that we removed paddocks.
    $paddocks_removed = TRUE;
  }

  // If paddocks were removed, tell the user what we did.
  if (!empty($paddocks_removed)) {
    drupal_set_message(t('Paddocks were removed from the plan. All rotations through those paddocks were deleted. Check your planned rotations and make any necessary adjustments.'), 'warning');
  }
}
