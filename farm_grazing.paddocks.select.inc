<?php

include_once "farm_grazing.utils.inc";

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

