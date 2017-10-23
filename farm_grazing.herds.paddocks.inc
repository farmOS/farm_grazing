<?php

include_once "farm_grazing.utils.inc";

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
