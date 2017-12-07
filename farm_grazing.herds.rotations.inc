<?php

/**
 * Periods page callback.
 */
function farm_grazing_plan_rotations_form($form, &$form_state, $plan_obj) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();

  // Set the page title.
  drupal_set_title(t('Create paddock rotations for each herd'));

  // Get a list of all herds associated with this plan.
  $query = db_query('SELECT h.*, a.name FROM {farm_grazing_herds} h JOIN {farm_asset} a ON h.herd_id=a.id WHERE plan_id=:plan_id ORDER BY a.name', array(':plan_id' => $plan));
  $herd_records = $query->fetchAll();

  // Get a list of all paddocks associated with this plan.
  $query = db_query('SELECT p.*, t.name FROM {farm_grazing_plan_paddock} p LEFT JOIN taxonomy_term_data t ON p.paddock_id=t.tid WHERE plan_id=:plan_id ORDER BY t.name', array(':plan_id' => $plan));
  $paddock_records = $query->fetchAll();

  // Validate the herds in the plan (silently). If error are detected, direct
  // the user back to the herd management page to correct them. Do not show
  // the rest of the rotations page.
  $errors = 0;
  foreach ($herd_records as $record) {
    $data = array();
    $errors += farm_grazing_validate_herd($record->herd_id, $record->plan_id, $data, TRUE);
  }
  if (empty($herd_records) || !empty($errors)) {
    $message = t('Some required data is missing from the herds/animals in this plan. Please correct these issues in the <a href="@herds_manage">Manage Herds</a> tab.', array('@herds_manage' => url('farm/plan/' . $plan . '/herds')));
    return array(
      'validation' => array(
        '#type' => 'markup',
        '#markup' => $message,
      ),
    );
  }

  $form['#tree'] = TRUE;

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );
  $form['text'] = array(
    '#markup' => t('
      <p>Drag and drop to reorder the paddocks for each herd. Avoid grazing during paddock exclusions or recovery periods.</p>'),
  );

  // Create an array of herd options for the select list.
  $herd_options = array();
  foreach ($herd_records as $rec) {
    $herd_options[$rec->herd_id] = $rec->name;
  }

  // Create an array of paddock options for the select list.
  $paddock_options = array();
  foreach ($paddock_records as $rec) {
    $paddock_options[$rec->paddock_id] = $rec->name;
  }

  // Add a fieldset with options for adding additional paddock rotations for a
  // given herd.
  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add a paddock rotation'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['herd_id'] = array(
    '#type' => 'select',
    '#title' => t('Herd'),
    '#description' => t('Select a herd you want to add a paddock to.'),
    '#options' => $herd_options,
  );
  $form['add']['paddock_id'] = array(
    '#type' => 'select',
    '#title' => t('Paddock'),
    '#description' => t('Select paddock(s) you want to add to this herd.'),
    '#options' => $paddock_options,
    '#multiple' => TRUE,
  );
  $form['add']['mode'] = array(
    '#type' => 'select',
    '#title' => t('Default Grazing days'),
    '#description' => t('Select whether to use minimum, average or maximum grazing days for the paddocks added.'),
    '#options' => array(
      '1' => t('Minimum grazing days'),
      '2' => t('Average grazing days'),
      '3' => t('Maximum grazing days'),
    ),
    '#default_value' => '2',
    '#required' => FALSE,
  );
  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Add rotation'),
    '#validate' => array('farm_grazing_plan_rotation_add_validate'),
    '#submit' => array('farm_grazing_plan_rotation_add_submit'),
  );

  if ($grazing_growing_season) {
    // build optional control to reset the number of grazing days
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
        '1' => t('Minimum grazing days'),
        '2' => t('Average grazing days'),
        '3' => t('Maximum grazing days'),
      ),
      '#required' => FALSE,
    );
    $form['reset_grazing_days']['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Update Grazing Days'),
      '#submit' => array('farm_grazing_plan_rotations_form_submit'),
    );
  }
  else {
    // get non-growing season grazing days and cache it here
    $ng_pad = farm_grazing_get_non_growing_summary_for_plan_id($plan, 1);
    $ng_grazing = array();
    for ($i = 0; $i < count($ng_pad) - 2; $i++) {
      $ng_grazing[$ng_pad[$i][11]] = $ng_pad[$i][4];
    }
  }

  // build sections for each herds associated with this plan
  $form['herds'] = array();

  $cache = NULL;

  // foreach herd
  foreach ($herd_records as $rec) {

    // create a section for the herd and list the paddocks assigned to it
    $form['herds'][$rec->herd_id]['title'] = array(
      '#markup' => '<h4>' . t('Rotations for herd ') . $rec->name . '</h4>',
    );

    // build a table of paddocks assigned to this herd
    $query = db_query('SELECT p.id, p.plan_id, p.herd_id, p.paddock_id,
        p.duration, p.start_date, p.log_id, p.weight as weight, t.name
      FROM {farm_grazing_rotations} p
      LEFT JOIN taxonomy_term_data t ON p.paddock_id=t.tid
      WHERE plan_id=:plan_id AND herd_id=:herd_id
      ORDER BY p.weight, p.start_date ASC',
      array(':plan_id' => $plan, ':herd_id' => $rec->herd_id));
    $paddocks = $query->fetchAll();
    $rowcnt = $query->rowCount();

    $arrival = farm_grazing_get_herd_earliest_arrival($rec->herd_id, $plan);
    $start_date = date_create_from_format('Y-m-d', $arrival);
    $i = 1;
    $cache = array();
    foreach ($paddocks as $pad) {
      if ($grazing_growing_season) {
        $grazing_days = farm_grazing_get_paddock_grazing_days($pad->duration, $pad->paddock_id, $plan, $rec->herd_id, $cache);
      }
      else {
        $grazing_days = $ng_grazing[$pad->paddock_id];
      }
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
          '#title' => t('Grazing days'),
          '#title_display' => 'invisible',
          '#default_value' => round($grazing_days, 1),
          '#size' => 10,
          '#maxlength' => 10,
        ),
        'start_date' => array(
          '#type' => 'textfield',
          '#title' => t('Start date'),
          '#title_display' => 'invisible',
          '#default_value' => $start_str,
          '#disabled' => TRUE,
        ),
        'end_date' => array(
          '#type' => 'textfield',
          '#title' => t('End date'),
          '#title_display' => 'invisible',
          '#default_value' => $end_str,
          '#disabled' => TRUE,
        ),
        'image' => array(
          '#markup' => '<img src="' . base_path() . 'farm/grazing/plan/' . $plan .
          '/plot/' . $pad->paddock_id . '/' . $start_str . '/' . $end_str .
          '">',
        ),
        'log_id' => array(
          '#type' => 'value',
          '#value' => !empty($pad->log_id) ? $pad->log_id : 0,
        ),
        'weight' => array(
          '#type' => 'weight',
          '#title' => t('Order'),
          '#default_value' => $pad->weight,
          '#delta' => $rowcnt,
          '#title_display' => 'invisible',
          '#attributes' => array(
            'class' => array('herd_' . $rec->herd_id . '_weight'),
          ),
        ),
      );
      if (!$grazing_growing_season) {
        $form['herds'][$rec->herd_id]['rotations'][$pad->id]['grazing_days']['#disabled'] = TRUE;
      }
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

  // Return markup.
  return $form;
}

function theme_farm_grazing_plan_rotations_form($variables) {

  // If the validation error message is set, bail.
  if (!empty($variables['form']['validation'])) {
    return;
  }

  $form =& $variables['form'];

  $plan = $form['plan']['#value'];

  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['text']);

  $output .= drupal_render($form['add']);
  $output .= drupal_render($form['reset_grazing_days']);

  if (!empty($form['herds'])) {
    foreach (element_children($form['herds']) as $herd_id) {
      // render the tables title
      $output .= drupal_render($form['herds'][$herd_id]['title']);

      // assemble the table rows
      $rows = array();
      if (!empty($form['herds'][$herd_id]['rotations'])) {
        foreach (element_children($form['herds'][$herd_id]['rotations']) as $rid) {
          $item =& $form['herds'][$herd_id]['rotations'][$rid];

          // Build movement log columns (link and done checkbox)
          $log_link = '';
          $log_done = '';
          if (!empty($item['log_id']['#value'])) {
            $log = log_load($item['log_id']['#value']);
            $uri = entity_uri('log', $log);
            $log_link = l(t('View log'), $uri['path']);
            if (!empty($log->done)) {
              $log_done = '✔';
            }
          }

          // Build a link to remove the rotation.
          $remove_link = l(t('Remove'), 'farm/plan/' . $plan . '/herds/rotations/' . $rid . '/delete', array('query' => array('destination' => current_path())));

          $class = array('draggable');
          $weight = drupal_render($item['weight']);
          if ($log_done == '✔') {
            $class = array();
            $remove_link = '';
            $item['grazing_days']['#disabled'] = TRUE;
            $weight = '';
          }

          // Assemble the row.
          $rows[] = array(
            'data' => array(
              drupal_render($item['name']),
              drupal_render($item['grazing_days']),
              drupal_render($item['start_date']),
              drupal_render($item['end_date']),
              drupal_render($item['image']),
              $log_link,
              $log_done,
              $remove_link,
              $weight,
            ),
            'class' => $class,
          );
        }
      }

      $header = array(
        t('Paddock'),
        t('Grazing Days'),
        t('Start Date'),
        t('End Date'),
        '<img src="' . base_path() . 'farm/grazing/plan/' . $plan . '/plot/0/0/0">',
        t('Movement Log'),
        t('Movement Done'),
        t('Remove'),
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
  }

  // render any remaining elements
  $output .= drupal_render_children($form);

  return $output;
}

/**
 * Rotations form submit.
 */
function farm_grazing_plan_rotations_form_submit($form, &$form_state) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  $plan_id = $form_state['values']['plan'];
  $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
  $plan_start_ts = $wrapper->field_farm_date_range->value->value();

  if ($form_state['values']['op'] == t('Save Changes')) {
    foreach ($form_state['values']['herds'] as $herd_id => $rotations) {
      foreach ($rotations['rotations'] as $rid => $data) {

        // Calculate the start date as a timestamp.
        $start_date = strtotime($data['start_date']);

        // Update the log.
        if (!empty($data['log_id'])) {
          $log = log_load($data['log_id']);
          if (!empty($log)) {
            $log->timestamp = $start_date;
            log_save($log);
          }
        }

        // Update the rotation record.
        db_update('farm_grazing_rotations')
          ->fields(array(
            'duration' => $data['grazing_days'],
            'weight' => $data['weight'],
            'start_date' => $start_date,
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
    $paddocks = farm_grazing_get_grazing_days_for_plan_id($plan_id, $d1, $d2, TRUE);
    $data = array();
    foreach ($paddocks as $p) {
      $data[$p[5]] = $p;
    }

    // update all rotation records in plan
    $records = db_query('SELECT id, paddock_id, herd_id, log_id FROM {farm_grazing_rotations} WHERE plan_id=:plan_id', array(':plan_id' => $plan_id));

    $cache = array();
    $start_date = $plan_start_ts;
    foreach ($records as $rec) {
      $days = farm_grazing_get_paddock_grazing_days(NULL, $rec->paddock_id, $plan_id, $rec->herd_id, $cache, $mode);

      // Update the log.
      if (!empty($rec->log_id)) {
        $log = log_load($rec->log_id);
        if (!empty($log)) {
          $log->timestamp = $start_date;
          log_save($log);
        }
      }

      // Update the rotation record.
      db_update('farm_grazing_rotations')
        ->fields(array(
          'duration' => $days,
          'start_date' => $start_date,
        ))
        ->condition('id', $rec->id)
        ->execute();

      // increment the paddock start_date by the duration in seconds
      $start_date += $days * 24 * 3600;
    }
  }
}

/**
 * Rotation add validate function.
 */
function farm_grazing_plan_rotation_add_validate($form, &$form_state) {

  // Make sure that both a herd and paddock(s) are selected.
  if (empty($form_state['values']['add']['herd_id'])) {
    form_set_error('add][herd_id', t('You must select a herd.'));
  }
  if (empty($form_state['values']['add']['paddock_id'])) {
    form_set_error('add][paddock_id', t('You must select paddocks.'));
  }
}

/**
 * Rotation add submit function.
 */
function farm_grazing_plan_rotation_add_submit($form, &$form_state) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  $plan_id = $form_state['values']['plan'];
  $herd_id = $form_state['values']['add']['herd_id'];
  $paddock_ids = $form_state['values']['add']['paddock_id'];
  $mode = $form_state['values']['add']['mode'];

  // Get the timestamp of the end of the last rotation.
  $result = db_query("SELECT duration, start_date, weight FROM {farm_grazing_rotations} WHERE plan_id=:plan_id AND herd_id=:herd_id ORDER BY weight DESC LIMIT 1", array(':plan_id' => $plan_id, ':herd_id' => $herd_id));
  $record = $result->fetchAssoc();

  // Calculate the start date.
  if (!empty($record)) {
    $start_date = $record['start_date'] + $record['duration'] * 24 * 3600;
  }

  // If a record doesn't exist, use the start date of the plan itself.
  else {
    $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
    $start_date = $wrapper->field_farm_date_range->value->value();
  }

  // get the weight of the next record to add
  $weight = $record['weight'] + 1;

  $cache = array();
  // Iterate through the paddocks and add rotation records.
  foreach ($paddock_ids as $i => $paddock_id) {

    // Load the herd.
    $herd = farm_asset_load($herd_id);

    // Load the paddock.
    $paddock = taxonomy_term_load($paddock_id);

    // get the paddock duration ie: num grazing days
    $duration = farm_grazing_get_paddock_grazing_days(NULL, $paddock_id, $plan_id, $herd_id, $cache, $mode);

    // Create movement log.
    $log = farm_movement_create($herd, array($paddock), $start_date, 'farm_activity', FALSE);

    // db_insert('farm_grazing_herd_paddocks')
    db_insert('farm_grazing_rotations')
      ->fields(array(
        'paddock_id' => $paddock_id,
        'plan_id' => $plan_id,
        'herd_id' => $herd_id,
        'duration' => $duration,
        'start_date' => $start_date,
        'log_id' => !empty($log->id) ? $log->id : 0,
        'weight' => $weight + $i,
      ))
      ->execute();

    // update the start_date for the next paddock by duration in seconds
    $start_date += $duration * 24 * 3600;
  }
}

/**
 * Rotation delete form.
 */
function farm_grazing_plan_rotation_delete_form($form, &$form_state, $plan, $rotation_id) {

  // Load the rotation record.
  $rotation = db_query('SELECT * FROM {farm_grazing_rotations} WHERE id = :id', array(':id' => $rotation_id))->fetch();

  // If the rotation doesn't exist, show page not found.
  if (empty($rotation)) {
    drupal_not_found();
    drupal_exit();
  }

  // Save the rotation ID to the form.
  $form['rotation_id'] = array(
    '#type' => 'value',
    '#value' => $rotation->id,
  );

  // Load the herd.
  $herd = farm_asset_load($rotation->herd_id);

  // Load the paddock.
  $paddock = taxonomy_term_load($rotation->paddock_id);

  // Build a return path.
  $path = drupal_get_destination();
  if ($path['destination'] == current_path()) {
    $plan_uri = entity_uri('farm_plan', $plan);
    $path = $plan_uri['path'];
  }

  // Build and return a confirmation form.
  return confirm_form($form,
    t('Are you sure you want to remove paddock %paddock_name from %herd_name?', array('%paddock_name' => $paddock->name, '%herd_name' => $herd->name)),
    $path,
    t('You can add it back later if you change your mind.'),
    t('Remove it!'),
    t('Cancel')
  );
}

/**
 * Rotation delete form submit.
 */
function farm_grazing_plan_rotation_delete_form_submit($form, &$form_state) {

  // If a rotation ID wasn't set, bail.
  if (empty($form_state['values']['rotation_id'])) {
    return;
  }
  $rotation_id = $form_state['values']['rotation_id'];

  // Delete log.
  $log_id = db_query('SELECT log_id FROM {farm_grazing_rotations} WHERE id = :id', array(':id' => $rotation_id))->fetchField();
  if (!empty($log_id)) {
    $log = log_load($log_id);
    if (!empty($log)) {
      log_delete($log);
    }
  }

  // get duration and start_date of record being deleted
  $result = db_query("SELECT duration, start_date, plan_id, herd_id FROM {farm_grazing_rotations} WHERE id=:id", array(':id' => $rotation_id));
  $rec_delete = $result->fetchAssoc();
  $plan_id = $rec_delete['plan_id'];
  $herd_id = $rec_delete['herd_id'];
  // In seconds.
  $duration = $rec_delete['duration'] * 24 * 3600;
  $start_date = $rec_delete['start_date'];

  // Delete the rotation record.
  db_delete('farm_grazing_rotations')
    ->condition('id', $rotation_id)
    ->execute();

  // for all records following this one
  $records = db_query("SELECT id, log_id, start_date FROM {farm_grazing_rotations} WHERE start_date>:start_date AND plan_id=:plan_id AND herd_id=:herd_id", array(':start_date' => $start_date, ':plan_id' => $plan_id, ':herd_id' => $herd_id));
  foreach ($records as $rec) {
    //   set the start_date = start_date - duration
    //   update database
    db_update('farm_grazing_rotations')
      ->expression('start_date', 'start_date - :duration', array(':duration' => $duration))
      ->condition('id', $rec->id)
      ->execute();

    //   update log
    $log = log_load($rec->log_id);
    if (!empty($log)) {
      $log->timestamp = $log->timestamp - $duration;
      log_save($log);
    }
  }
}
