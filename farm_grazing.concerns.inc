<?php

include_once "farm_grazing.utils.inc";

/**
 * Management concerns form callback.
 */
function farm_grazing_plan_concerns_form($form, &$form_state, $plan_obj) {

  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();
  $farm_grazing_factors = $wrapper->field_farm_grazing_factors->value();
  $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
  $days_of_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
  $expected_rotations = $wrapper->field_expected_rotations->value();
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  $expected_days_non_growth = floor(($end_ts-$start_ts)/3600/24);


  //dpm('grazing_growing_season: ' . $grazing_growing_season);
  //kpr($farm_grazing_factors);
  //dpm('farm_grazing_factors: ' . $farm_grazing_factors['value']);
  //dpm('days_bulk_feeding: ' . $days_bulk_feeding);
  //dpm('days_of_drought_reserve: ' . $days_of_drought_reserve);
  //dpm('expected_days_non_growth: ' . $expected_days_non_growth);
  //dpm('expected_rotations: ' . $expected_rotations);

  if ($grazing_growing_season) {
    $growing_season = t('A Growing Season Plan');
  }
  else {
    $growing_season = t('A Non-Growing Season Plan');
  }

  // Set the page title.
  drupal_set_title(t('Management concerns'));

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  $form['section_1']['text'] =  array(
    '#markup' => '<h2>' . $growing_season . '</h2>' .
      '<p>' . t('In this section, think about various factors that influence your calendar for the plan. Consider:') . '</p>' .
      '<ul>' .
          '<li>' . t('Livestock events like birthing, breeding, weaning, etc and when these are planned.') .
          '<li>' . t('Management events such as vacations, family or community events that need to be planned around.') .
          '<li>' . t('Other specific events like hunting seasons, parasites, or flooding that might influence the sequence of paddock moves.') .
      '</ul>');

  $form['section_2']['factors'] = array(
      '#title' => t('Planning factors'),
      '#type' => 'textarea',
      '#description' => t('Enter your planning factors here'),
      '#default_value' => $farm_grazing_factors['value'],
  );

  $form['factors_format'] = array(
    '#type' => 'value',
    '#value' => $farm_grazing_factors['format'],
  );

  if (! $grazing_growing_season) {
    $form['non_growing'] = array(
      '#type' => 'fieldset',
      '#title' => t('Non-growing plan variables'),
      '#description' => t('Enter the follow parameters for the non-growing season plan.'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
    );
    $form['non_growing']['days_bulk_feeding'] = array(
      '#type' => 'textfield',
      '#title' => t('Days of bulk feeding'),
      '#default_value' => $days_bulk_feeding,
      '#required' => TRUE,
    );
    $form['non_growing']['days_of_drought_reserve'] = array(
      '#type' => 'textfield',
      '#title' => t('Days of drought reserve required'),
      '#default_value' => $days_of_drought_reserve,
      '#required' => TRUE,
    );
    $form['non_growing']['expected_days_non_growth'] = array(
      '#type' => 'textfield',
      '#title' => t('Expected days of non-growth'),
      '#default_value' => $expected_days_non_growth,
      '#disabled' => TRUE,
    );
    $form['non_growing']['expected_rotations'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of planned rotations through paddocks'),
      '#default_value' => $expected_rotations,
      '#required' => TRUE,
    );
  }

  // TODO - Need to add management events and integrate with calendar

  /*
  $form['section_4']['text'] = array(
    '#markup' => '<h2>' . t('Management Events') . '</h2>' .
      '<p>' . t('Enter management events into the Calendar -- TODO') . '</p>',
  );
  */

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
    '#submit' => array('farm_grazing_plan_concerns_form_submit'),
    '#validate' => array('farm_grazing_plan_concerns_form_validate'),
  );


  return $form;
}

/**
 * theme_farm_grazing_plan_concerns_form
 *
 * theme handler for farm_grazing_plan_concerns_form
 */
function theme_farm_grazing_plan_concerns_form($variables) {
  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($variables, true));

  $form = &$variables['form'];
  $plan = $form['plan']['#value'];
  //$growing_season = $form['growing_season']['#value'];

  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['section_1']['text']);
  $output .= drupal_render($form['section_2']['factors']);
  $output .= drupal_render($form['factors_format']);

  if (array_key_exists($form, 'non_growing')) {
    $output .= drupal_render($form['non_growing']);
  }

  $output .= drupal_render_children($form);

  return $output;
}


/**
 * farm_grazing_plan_concerns_form_validate
 *
 * validate handler for farm_grazing_plan_concerns_form
 */
function farm_grazing_plan_concerns_form_validate($form, &$form_state) {
  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['input'], true));

  if (! is_numeric($form_state['input']['days_bulk_feeding']) or
      $form_state['input']['days_bulk_feeding'] < 0) {
    form_set_error('days_bulk_feeding', t('Days bulk feeding must be numeric >= 0'));
  }
  if (! is_numeric($form_state['input']['days_of_drought_reserve']) or
      $form_state['input']['days_of_drought_reserve'] < 0) {
    form_set_error('days_of_drought_reserve', t('Days of drought reserve required must be numeric >= 0'));
  }
  if (! is_numeric($form_state['input']['expected_rotations']) or
      $form_state['input']['expected_rotations'] < 0) {
    form_set_error('expected_rotations', t('Number of planned rotations must be numeric >= 0'));
  }
}

/**
 * farm_grazing_plan_concerns_form_submit
 *
 * submit handler for farm_grazing_plan_concerns_form
 */
function farm_grazing_plan_concerns_form_submit($form, &$form_state) {
  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['values'], true));

  $wrapper = entity_metadata_wrapper('farm_plan', $form_state['values']['plan']);

  if (isset($form_state['values']['factors'])) {
    $wrapper->field_farm_grazing_factors->set(array(
      'value' => $form_state['values']['factors'],
    ));
  }
  if (isset($form_state['values']['days_bulk_feeding'])) {
    $wrapper->field_days_bulk_feeding->set($form_state['values']['days_bulk_feeding']);
  }
  if (isset($form_state['values']['days_of_drought_reserve'])) {
    $wrapper->field_days_of_drought_reserve->set($form_state['values']['days_of_drought_reserve']);
  }
  if (isset($form_state['values']['expected_rotations'])) {
    $wrapper->field_expected_rotations->set($form_state['values']['expected_rotations']);
  }

  $wrapper->save();
}
