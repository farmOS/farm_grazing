<?php

include_once "farm_grazing.utils.inc";

/**
 * Periods page callback.
 */
function farm_grazing_plan_rotations_form($form, &$form_state, $plan_obj) {

  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();

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

  if ($grazing_growing_season) {
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
  }
  else {
    // get non-growing season grazing days and cache it here
    $ng_pad = getNonGrowingSummaryForPlanId($plan, 1);
    $ng_grazing = array();
    for ($i=0; $i<count($ng_pad)-2; $i++) {
      $ng_grazing[$ng_pad[$i][11]] = $ng_pad[$i][4];
    }
    //kpr($ng_pad);
    //kpr($ng_grazing);
  }

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
        p.duration, p.start_date, p.log_id, p.weight as weight, t.name
      FROM {farm_grazing_rotations} p
      LEFT JOIN taxonomy_term_data t ON p.paddock_id=t.tid
      WHERE plan_id=:plan_id AND herd_id=:herd_id
      ORDER BY p.weight, p.start_date ASC',
      array(':plan_id' => $plan, ':herd_id' => $rec->herd_id));
    $paddocks = $query->fetchAll();
    $rowcnt = $query->rowCount();

    $rows = array();
    $arrival = getHerdEarliestArrival($rec->herd_id);
    $start_date = date_create_from_format('Y-m-d', $arrival);
    $i = 1;
    foreach ($paddocks as $pad) {
      if ($grazing_growing_season) {
        $grazing_days = getPaddockGrazingDays($pad->duration, $pad->paddock_id, $plan, $rec->herd_id, $minOrMax);
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
          '#markup' => '<img src="' . base_path() . 'farm/grazing/plan/' .  $plan .
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
      if (! $grazing_growing_season) {
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
      '<img src="' . base_path() . 'farm/grazing/plan/'.$plan.'/plot/0/0/0">',
      t('Movement Log'),
      t('Movement Done'),
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
          $log->timestamp = $start_date;
          log_save($log);
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
    $paddocks = getGrazingDaysForPlanId($plan_id, $d1, $d2, true);
    $data = array();
    foreach ($paddocks as $p) {
      $data[$p[5]] = $p;
    }

    // update all rotation records in plan
    $records = db_query('SELECT id, paddock_id, herd_id, log_id FROM {farm_grazing_rotations} WHERE plan_id=:plan_id', array(':plan_id' => $plan_id));

    $cache = array();
    $start_date = $plan_start_ts;
    foreach ($records as $rec) {
      $days = getPaddockGrazingDays(NULL, $rec->paddock_id, $plan_id, $rec->herd_id, $cache, $mode);

      // Update the log.
      if (!empty($rec->log_id)) {
        $log = log_load($rec->log_id);
        $log->timestamp = $start_date;
        log_save($log);
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


