<?php

include_once "farm_grazing.utils.inc";

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

