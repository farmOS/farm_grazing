<?php

/**
 * @file
 * Grazing rotations form.
 */

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
  foreach ($herd_records as $herd) {
    $data = array();
    $errors += farm_grazing_validate_herd($herd->herd_id, $herd->plan_id, $data, TRUE);
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
  foreach ($herd_records as $herd) {
    $herd_options[$herd->herd_id] = $herd->name;
  }

  // Create an array of paddock options for the select list.
  $paddock_options = array();
  foreach ($paddock_records as $paddock) {
    $paddock_options[$paddock->paddock_id] = $paddock->name;
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

  // build sections for each herds associated with this plan
  $form['herds'] = array();

  // Keep track of whether or not there are rotations in the form.
  $rotations_exist = FALSE;

  // Iterate through the herds.
  foreach ($herd_records as $herd) {

    // create a section for the herd and list the paddocks assigned to it
    $form['herds'][$herd->herd_id]['title'] = array(
      '#markup' => '<h4>' . t('Rotations for herd %herd', array('%herd' => $herd->name)) . '</h4>',
    );

    // Query all the paddock rotations for this plan+herd.
    $rotations = farm_grazing_load_rotations($plan, $herd->herd_id);
    $count_rotations = count($rotations);

    // Iterate through the paddock rotations.
    $weight = 0;
    foreach ($rotations as $rotation) {

      // Load the log that is linked to the rotation.
      $log = log_load($rotation->log_id);

      // Figure out the start and end date strings.
      $start_str = date('Y-m-d', $rotation->start_date);
      $end_str = date('Y-m-d', $rotation->start_date + ($rotation->duration * 86400));

      // The grazing days field should be disabled if:
      //   a) the plan is not a growing season, or:
      //   b) the log is done
      $grazing_days_disabled = !$grazing_growing_season || !empty($log->done);

      // Add form fields for the rotation details.
      $form['herds'][$herd->herd_id]['rotations'][$rotation->id] = array(
        'name' => array(
          '#markup' => $rotation->name,
        ),
        'grazing_days' => array(
          '#type' => 'textfield',
          '#title' => t('Grazing days'),
          '#title_display' => 'invisible',
          '#default_value' => $rotation->duration,
          '#size' => 10,
          '#maxlength' => 10,
          '#disabled' => $grazing_days_disabled,
        ),
        'start_date' => array(
          '#type' => 'value',
          '#value' => $start_str,
        ),
        'end_date' => array(
          '#type' => 'value',
          '#value' => $end_str,
        ),
        'date_range' => array(
          '#markup' => t('@start_date to @end_date', array('@start_date' => $start_str, '@end_date' => $end_str)),
        ),
        'image' => array(
          '#markup' => '<img src="' . base_path() . 'farm/grazing/plan/' . $plan .
          '/plot/' . $rotation->paddock_id . '/' . $start_str . '/' . $end_str .
          '">',
        ),
        'log_id' => array(
          '#type' => 'value',
          '#value' => !empty($rotation->log_id) ? $rotation->log_id : 0,
        ),
        'weight' => array(
          '#type' => 'weight',
          '#title' => t('Order'),
          '#default_value' => $weight,
          '#delta' => $count_rotations,
          '#title_display' => 'invisible',
          '#attributes' => array(
            'class' => array('herd_' . $herd->herd_id . '_weight'),
          ),
        ),
      );

      // Keep track of the fact that there are rotations in the form.
      $rotations_exist = TRUE;

      // Increment the weight.
      $weight++;
    }
  } // end foreach herd

  // If there are no rotations, stop here. We won't show the form options for
  // recalculating, because there's nothing to recalculate.
  if (!$rotations_exist) {
    return $form;
  }

  // Add options for recalculating rotations.
  $form['recalculate'] = array(
    '#type' => 'fieldset',
    '#title' => t('Recalculate all rotations'),
    '#description' => t('This will recalculate the start and end dates of all rotations in this plan, and optionally reset the grazing days for each. All "Done" movements will not be affected.'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );

  // If this is a growing season plan, add control to reset the number of
  // grazing days on all rotations.
  if ($grazing_growing_season) {
    $form['recalculate']['reset_grazing_days'] = array(
      '#type' => 'select',
      '#title' => t('Reset grazing days?'),
      '#description' => t('This will reset the grazing days for all rotations in this plan to either the minimum, average, or maximum for each paddock.'),
      '#options' => array(
        0 => t('No change'),
        1 => t('Minimum'),
        2 => t('Average'),
        3 => t('Maximum'),
      ),
      '#default_value' => 0,
      '#required' => TRUE,
    );
  }

  // Submit button.
  $form['recalculate']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Recalculate'),
  );

  // Return the form.
  return $form;
}

/**
 * Theme function for the rotations form.
 */
function theme_farm_grazing_plan_rotations_form($variables) {

  // If the validation error message is set, bail.
  if (!empty($variables['form']['validation'])) {
    return;
  }

  drupal_add_css('svg {position:absolute; background:white;}', 'inline');
  drupal_add_css('td img, th img {position:relative; width:100%; height:100%; min-width: 200px;}', 'inline');

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

          // Start an array for action links.
          $actions = array();

          // Start an array for row classes.
          $classes = array();

          // Load the rotation log.
          if (!empty($item['log_id']['#value'])) {
            $log = log_load($item['log_id']['#value']);
          }

          // If the log is missing or didn't load, make it clear to the user
          // (in the "Done" column).
          if (!empty($log)) {

            // Add an action link to view the log.
            $uri = entity_uri('log', $log);
            $actions[] = l(t('View log'), $uri['path']);

            // If the log is done, add a checkbox.
            if (!empty($log->done)) {
              $log_done = '✔';
            }

            // Or, if the log is not done, leave the "Done" column blank, and
            // make the row draggable.
            else {
              $log_done = '';
              $classes[] = 'draggable';
            }
          }

          // If the log is missing or didn't load, make it clear to the user
          // (in the "Done" column).
          else {
            $log_done = '(log missing)';
          }

          // If there is no log, or if the log is not done, add an action link
          // for removing the rotation.
          if (empty($log) || empty($log->done)) {
            $actions[] = l(t('Remove'), 'farm/plan/' . $plan . '/herds/rotations/' . $rid . '/delete', array('query' => array('destination' => current_path())));
          }

          // Assemble actions links.
          $actions_links = implode(' | ', $actions);

          // Assemble the row.
          $rows[] = array(
            'data' => array(
              drupal_render($item['name']),
              drupal_render($item['grazing_days']),
              drupal_render($item['date_range']),
              drupal_render($item['image']),
              $log_done,
              $actions_links,
              drupal_render($item['weight']),
            ),
            'class' => $classes,
          );
        }
      }

      $header = array(
        array(
          'data' => t('Paddock'),
          'class' => array('col-xs-1'),
        ),
        array(
          'data' => t('Grazing Days'),
          'class' => array('col-xs-1'),
        ),
        array(
          'data' => t('Date Range'),
          'class' => array('col-xs-1'),
        ),
        array(
          'data' => '<img src="' . base_path() . 'farm/grazing/plan/' . $plan . '/plot/0/0/0">',
          'class' => array('col-xs-5'),
        ),
        array(
          'data' => t('Done'),
          'class' => array('col-xs-1'),
        ),
        array(
          'data' => t('Actions'),
          'class' => array('col-xs-2'),
        ),
        array(
          'data' => t('Sort weight'),
          'class' => array('col-xs-1'),
        ),
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

  // Get the plan ID.
  $plan_id = $form_state['values']['plan'];

  // Get the reset rotations mode, if available.
  $reset_rotations = 0;
  if (!empty($form_state['values']['recalculate']['reset_grazing_days'])) {
    $reset_rotations = $form_state['values']['recalculate']['reset_grazing_days'];
  }

  // If there are herds in the form, iterate through them.
  if (!empty($form_state['values']['herds'])) {
    foreach ($form_state['values']['herds'] as $herd_id => $herd_form_values) {

      // Load all rotations for the herd in this plan.
      $rotations = farm_grazing_load_rotations($plan_id, $herd_id);

      // If there are no rotations, skip this herd.
      if (empty($rotations)) {
        continue;
      }

      // If there are rotations in the herd form...
      if (!empty($herd_form_values['rotations'])) {

        // Keep track of whether or not the order of rotations is changing.
        $reorder = FALSE;

        // Iterate through the rotations.
        foreach ($herd_form_values['rotations'] as $rotation_id => $rotation_form_values) {

          // If the grazing days have been changed, save it to the rotation for
          // recalculation after this.
          $duration = $rotation_form_values['grazing_days'];
          $duration_default = $form['herds'][$herd_id]['rotations'][$rotation_id]['grazing_days']['#default_value'];
          if ($duration != $duration_default) {
            $rotations[$rotation_id]->duration = $duration;
          }

          // Add the weight to the rotation. If the user changed it, trigger a
          // reorder of the rotations.
          $weight = $rotation_form_values['weight'];
          $weight_default = $form['herds'][$herd_id]['rotations'][$rotation_id]['weight']['#default_value'];
          $rotations[$rotation_id]->weight = $weight;
          if ($rotations[$rotation_id]->weight != $weight_default) {
            $reorder = TRUE;
          }
        }

        // Reorder rotations, if desired.
        if ($reorder) {
          uasort($rotations, 'farm_grazing_rotation_sort');
        }
      }

      // Recalculate all rotations for the herd in this plan.
      farm_grazing_recalculate_rotations($plan_id, $herd_id, $rotations, $reset_rotations);
    }
  }
}

/**
 * Rotation sort callback.
 */
function farm_grazing_rotation_sort($a, $b) {
  if ($a->weight == $b->weight) {
    return 0;
  }
  return ($a->weight < $b->weight) ? -1 : 1;
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

  // Get the last rotation for the herd in this plan.
  $last_rotation = db_query("SELECT * FROM {farm_grazing_rotations} WHERE plan_id=:plan_id AND herd_id=:herd_id ORDER BY start_date DESC LIMIT 1", array(':plan_id' => $plan_id, ':herd_id' => $herd_id))->fetch();

  // If a rotation doesn't exist, load the timestamp when the plan starts.
  if (empty($last_rotation)) {
    $plan_wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
    $timestamp = $plan_wrapper->field_farm_date_range->value->value();
  }

  // Otherwise, calculate the timestamp when this rotation ends.
  else {

    // Convert the duration to seconds.
    $duration_seconds = $last_rotation->duration * 86400;

    // Add it to the start time of the rotation.
    $timestamp = $last_rotation->start_date + $duration_seconds;
  }

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
    $log = farm_movement_create($herd, array($paddock), $timestamp, 'farm_activity', FALSE);

    // Write the new rotation record to the database.
    $rotation = array(
      'paddock_id' => $paddock_id,
      'plan_id' => $plan_id,
      'herd_id' => $herd_id,
      'duration' => $duration,
      'start_date' => $timestamp,
      'log_id' => !empty($log->id) ? $log->id : 0,
    );
    drupal_write_record('farm_grazing_rotations', $rotation);

    // Update the start_date for the next paddock by duration in seconds.
    $timestamp += $duration * 24 * 3600;
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

  // Save the rotation to the form.
  $form['rotation'] = array(
    '#type' => 'value',
    '#value' => $rotation,
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

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // If a rotation wasn't set, bail.
  if (empty($form_state['values']['rotation'])) {
    return;
  }
  $rotation = $form_state['values']['rotation'];

  // Get the plan ID and herd ID.
  $plan_id = $rotation->plan_id;
  $herd_id = $rotation->herd_id;

  // Delete log.
  if (!empty($rotation->log_id)) {
    $log = log_load($rotation->log_id);
    if (!empty($log)) {
      log_delete($log);
    }
  }

  // Delete the rotation record.
  db_delete('farm_grazing_rotations')
    ->condition('id', $rotation->id)
    ->execute();

  // Recalculate all the remaining rotations.
  farm_grazing_recalculate_rotations($plan_id, $herd_id);
}
