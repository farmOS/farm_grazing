<?php

include_once "farm_grazing.utils.inc";

/**
 * Management concerns page callback.
 */
function farm_grazing_plan_concerns_form($form, &$form_state, $plan_obj) {

  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();
  $farm_grazing_factors = $wrapper->field_farm_grazing_factors->value();
  $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
  $days_of_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
  $expected_days_non_growth = $wrapper->field_expected_days_non_growth->value();
  $expected_rotations = $wrapper->field_expected_rotations->value();

  dpm('grazing_growing_season: ' . $grazing_growing_season);
  kpr('farm_grazing_factors: ' . $farm_grazing_factors);
  dpm('days_bulk_feeding: ' . $days_bulk_feeding);
  dpm('days_of_drought_reserve: ' . $days_of_drought_reserve);
  dpm('expected_days_non_growth: ' . $expected_days_non_growth);
  dpm('expected_rotations: ' . $expected_rotations);

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
  $form['growing_season'] = array(
    '#type' => 'value',
    '#value' => $grazing_growing_season,
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
      '#default_value' => $farm_grazing_factors,
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
      '#title' => t(''),
      '#default_value' => $days_bulk_feeding,
      '#required' => TRUE,
    );
    $form['non_growing']['days_of_drought_reserve'] = array(
      '#type' => 'textfield',
      '#title' => t(''),
      '#default_value' => $days_of_drought_reserve,
      '#required' => TRUE,
    );
    $form['non_growing']['expected_days_non_growth'] = array(
      '#type' => 'textfield',
      '#title' => t(''),
      '#default_value' => $expected_days_non_growth,
      '#required' => TRUE,
    );
    $form['non_growing']['expected_rotations'] = array(
      '#type' => 'textfield',
      '#title' => t(''),
      '#default_value' => $expected_rotations,
      '#required' => TRUE,
    );
  }

  $form['section_4']['text'] = array(
    '#markup' => '<h2>' . t('Management Events') . '</h2>' .
      '<p>' . t('Enter management events into the Calendar on the "Calendar" tab. Enter other events into your ') . '<a href="' . base_path() . 'farm/plan/' . $plan . '/edit">' . t('Planning Factors') . '</a>' . t(' for the plan for now.') . '</p>' .
      '<p>' . t('You will come back to this later when you need to enter paddock exclusions and are planning your moves as it will be helpful to be able to see these events while planning moves.') . '</p>'
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
    '#submit' => array('farm_grazing_plan_concerns_form_submit'),
  );


  return $form;
}

function theme_farm_grazing_plan_concerns_form($variables) {
  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($variables, true));

  $form = &$variables['form'];
  $plan = $form['plan']['#value'];
  $growing_season = $form['growing_season']['#value'];

  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['section_1']['text']);
  $output .= drupal_render($form['section_2']['factors']);

  if (array_key_exists($form, 'non_growing')) {
    $output .= drupal_render($form['non_growing']);
  }

  $output .= drupal_render_children($form);

  return $output;
}

function farm_grazing_plan_concerns_form_submit($form, &$form_state) {

}
