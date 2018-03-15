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
  $form['description'] = array(
    '#markup' => '<div class="fg_description"></div>',
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

  // fetch all considerations that impact this plan
  $considerations = farm_grazing_get_plan_considerations($plan_obj);

  // Iterate through the herds.
  foreach ($herd_records as $herd) {

    // create a section for the herd and list the paddocks assigned to it
    $form['herds'][$herd->herd_id]['title'] = array(
      '#markup' => '<h4>' . t('Rotations for herd %herd', array('%herd' => $herd->name)) . '</h4>',
    );

    // setup the considerations that are not related to a paddock
    // one idea was to put them into a separate table from the paddock rotations
    // but getting them to align correctly with the following table seems problematic
    if (!empty($considerations) and !empty($considerations[0])) {
        foreach ($considerations[0] as $consider) {
            // add a row for the consideration
            // TODO
        }
    }

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

      // compute the SVG image data for this rotation
      $svg = farm_grazing_get_rotation_svg($plan_obj, $rotation->paddock_id, $considerations, $start_str, $end_str);

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
          '#markup' => $svg,
        ),
        // TODO this should be empty of only consideration
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
  drupal_add_css('td svg, th svg {position:relative; width:100%; height:100%; min-width: 200px;}', 'inline');

    // TODO
    // This css and the following JS  are for doing tooltips over the rectangles in the SVG images
    // But this does not work with SVG in an IMG tag. So, the code needs to generate the SVG directly
    // into the TD and see if that works and if it does probably we should move the CSS and JS in files.
 
  drupal_add_css('
.heyo:hover {
  fill: #CC2929;
  -moz-transition: 0.3s;
  -o-transition: 0.3s;
  -webkit-transition: 0.3s;
  transition: 0.3s;
}

.enabled {
  cursor: pointer;
}

.fg_description {
  pointer-events: none;
  position: absolute;
  font-size: 18px;
  text-align: center;
  vertical-align: top;
  background: white;
  padding: 10px 15px;
  z-index: 5;
  height: 30px;
  line-height: 4px;
  margin: 0 auto;
  color: #21669e;
  border-radius: 5px;
  border-color: black;
  border-style: solid;
  border-width: thin;
  box-shadow: 0 0 0 1px #eee;
  -moz-transform: translateX(-50%);
  -ms-transform: translateX(-50%);
  -webkit-transform: translateX(-50%);
  transform: translateX(-50%);
  display: none;
}
.fg_description.active {
  display: block;
}
.fg_description:after {
  content: "";
  position: absolute;
  left: 50%;
  top: 100%;
  width: 0;
  height: 0;
  margin-left: -10px;
  border-left: 10px solid transparent;
  border-right: 10px solid transparent;
  border-top: 10px solid white;
}
', 'inline');

drupal_add_js('jQuery(document).ready(function() {
  $description = jQuery(".fg_description");

  jQuery(\'.enabled\').hover(function() {
          
    jQuery(this).attr("class", "enabled heyo");
    $description.addClass(\'active\');
    $description.html(jQuery(this).attr(\'alt\'));
  }, function() {
    $description.removeClass(\'active\');
  });

jQuery(document).on(\'mousemove\', function(e){
  
  $description.css({

    left:  e.pageX,
    top:   e.pageY - 180
  });
  
});
});
', 'inline');

  $form =& $variables['form'];

  $plan = $form['plan']['#value'];
  $plan_obj = farm_plan_load($plan);

  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['description']);
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

          // Build information about the log.
          $log_info = '';
          if (!empty($log)) {

            // Add an action link to view the log.
            $uri = entity_uri('log', $log);
            $log_info = l(t('View log'), $uri['path']);

            // If the log is done, add a checkbox.
            if (!empty($log->done)) {
              $log_info .= ' (✔)';
            }

            // Or, if the log is not done, make the row draggable.
            else {
              $classes[] = 'draggable';
            }
          }

          // If the log is missing or didn't load, make it clear to the user.
          // TODO unless this is a consideration only
          else {
            $log_info = '(log missing)';
          }

          // Add log info as an action item.
          $actions[] = $log_info;

          // If there is no log, or if the log is not done, add an action link
          // for removing the rotation.
          if (empty($log) || empty($log->done)) {
            $actions[] = l(t('Remove'), 'farm/plan/' . $plan . '/herds/rotations/' . $rid . '/delete', array('query' => array('destination' => current_path())));
          }

          // Assemble actions links.
          $actions_links = implode(' | ', $actions);

          // Assemble the row.
          // TODO name and grazing_days need colspan="2" for consideration only rows
          $rows[] = array(
            'data' => array(
              drupal_render($item['name']),
              drupal_render($item['grazing_days']),
              drupal_render($item['date_range']),
              drupal_render($item['image']),
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
          'data' => farm_grazing_get_rotation_svg($plan_obj, 0, array(), 0, 0),
          'class' => array('col-xs-7'),
        ),
        array(
          'data' => t('Actions'),
          'class' => array('col-xs-1'),
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

  // Print a message.
  drupal_set_message(t('All rotations have been saved.'));
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

  // Print a message.
  drupal_set_message(t('The rotation has been deleted. All rotations after it were automatically recalculated.'));
}

/**
 * Fetch considerations for plan and return a key/value array of the records
 * where the key is the paddock_id and the value is an array of records
 * a key=0 is used for considerations NOT linked to any specific paddock
 *
 * @param $plan_obj - the plan to fetch considerations for
 * @return array(paddock_id => array(records))
 *
 * record->entity_id
 *       ->entity_type
 *       ->type
 *       ->start
 *       ->end
 *       ->name
 *
 */
function farm_grazing_get_plan_considerations($plan_obj) {
    $wrapper = entity_metadata_wrapper('farm_plan', $plan_obj->id);
    $plan_start = $wrapper->field_farm_date_range->value->value();
    $plan_end = $wrapper->field_farm_date_range->value2->value();

    // we need to select considerations that overlap the plan start and end dates
    //                   start           end
    //                     |----plan------|
    //          |---|   |====| |=====|  |====| |----|
    //
    // in the cases above we want the considerations represented  by |===| and not
    // those represented by |---|
    //

    $sql = "SELECT e.entity_id, e.entity_type, c.type, c.start, c.end, c.name
        FROM {farm_plan_consideration} c
        LEFT JOIN {farm_plan_consideration_entity} e
        ON c.id=e.consideration_id
        WHERE c.start <= :plan_end AND c.end >= :plan_start
        AND ( c.plan_id=:plan_id OR c.plan_id IS NULL)";

    $records = db_query($sql, array(':plan_start' => $plan_start, ':plan_end' => $plan_end, ':plan_id' => $plan_obj->id));

    $considerations = array();
    foreach ($records as $record) {
        $id = empty($record->entity_id) ? 0 : $record->entity_id;
        $considerations[$id][] = $record;
    }

    return $considerations;
}

/**
 * farm_grazing_get_rotation_svg($plan_obj, $paddock_id, $considerations, $start_str, $end_str)
 */
function farm_grazing_get_rotation_svg($plan_obj, $paddock_id, $considerations, $start_str, $end_str) {
    module_load_include('inc', 'farm_grazing', 'farm_grazing.herds.rotations.plot');

    /*
    $plan_id =$plan_obj->id;
    $svg = '<img src="' . base_path() . 'farm/grazing/plan/' . $plan_id .
      '/plot/' . $paddock_id . '/' . $start_str . '/' . $end_str .
      '">';
     */
    $svg = farm_grazing_plan_plot_svg($plan_obj, $paddock_id, $considerations, $start_str, $end_str);
    return $svg;
}
