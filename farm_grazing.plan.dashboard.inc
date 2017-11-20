<?php

include_once "farm_grazing.utils.inc";

/**
 * Management concerns form callback.
 */
function farm_grazing_plan_dashboard_form($form, &$form_state, $plan_obj) {

  $plan = $plan_obj->id;
  $wrapper = entity_metadata_wrapper('farm_plan', $plan);

  $grazing_growing_season = $wrapper->field_grazing_growing_season->value();
  $farm_grazing_factors = $wrapper->field_farm_grazing_factors->value();
  $days_bulk_feeding = $wrapper->field_days_bulk_feeding->value();
  $days_of_drought_reserve = $wrapper->field_days_of_drought_reserve->value();
  $expected_rotations = $wrapper->field_expected_rotations->value();
  $start_ts = $wrapper->field_farm_date_range->value->value();
  $end_ts = $wrapper->field_farm_date_range->value2->value();

  $expected_days_non_growth = floor(($end_ts - $start_ts) / 3600 / 24);


  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  if (!$grazing_growing_season) {
    $form['non_growing'] = array(
      '#type' => 'value',
      '#value' => !$grazing_growing_season,
    );
  }

  $form['spacer_1'] = array(
    '#prefix' => '<br><p>',
    '#markup' => t('Click the sections below to open/close them.'),
    '#suffix' => '</p><br>',
    '#weight' => 101,
  );

  // Add introductory text.
  $form['introduction'] = array(
    '#type' => 'fieldset',
    '#title' => t('Getting Started'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 105,
  );

  // get the path to the herds rotations we use below
  $current_path = base_path() . current_path();
  $rotation_url = trim($current_path, "#") . '/herds/rotations';
  $animal_types_url = base_path() . 'admin/config/farm/grazing_animal_types';

  $form['introduction']['text'] = array(
    '#type' => 'markup',
    '#markup' => '<p>' . t('Welcome to the Holistic Grazing Planning Application.') . '</p>' .
    '<h4>' . t('For the First Time') . '</h4>' .
    '<p>' . t('To get started, you need to:') .
    '<ul>' .
    '<li>' . t('Define your ranch\'s paddocks. This is done on the "Areas" tab at the top of the page. This needs to be done once initially and anytime you decide to change the layout of paddocks on the ranch.') . '</li>' .
    '<li>' . t('Review and add Animal types ') .
    '<a href="' . $animal_types_url . '">' . t('HERE') . '</a>' . t(' that you will be working with.') . '</li>' .
    '</ul>' .
    '</p>' .
    '<h4>' . t('Making a Plan') . '</h4>' .
    '<p>' . t('This is the "active" plan you are working with. The planning process follows the order of the tabs from left to right and top to bottom. You can revisit any tab as needed to review or change your inputs. Most of the tabs are focused on collecting data needed to build your plan. Planning the paddock rotations is done on ') . '<a href="' . $rotation_url . '">' . t('Herds/Rotations') . '</a>' . t(' tab.') . '</p>' .
    '<p>' . t('The Plan tab will bring you back here and the Settings tab allows you edit information about the plan.') . '</p>' .
    '<p>' . t('Next Steps: Simple open the "Management Concerns" section below and enter data there and then work your way across the Paddocks and Herds tabs both have sub-menu steps that you should work left to right.') . '</p>',
  );

  $form['concerns'] = array(
    '#type' => 'fieldset',
    '#title' => t('Management Concerns'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 110,
  );

  $form['concerns']['section_1']['text'] = array(
  // '<h2>Management concerns</h2>' .
    '#markup' =>
    '<p>' . t('In this section, think about various factors that influence your calendar for the plan. Consider:') . '</p>' .
    '<ul>' .
    '<li>' . t('Livestock events like birthing, breeding, weaning, etc and when these are planned.') .
    '<li>' . t('Management events such as vacations, family or community events that need to be planned around.') .
    '<li>' . t('Other specific events like hunting seasons, parasites, or flooding that might influence the sequence of paddock moves.') .
    '<li>' . t('Enter these concerns below. You will want to add to this and review it as you are doing your planning and planning the herd rotations.') .
    '</ul>',
  );

  $form['concerns']['section_2']['factors'] = array(
    '#title' => t('Planning factors'),
    '#type' => 'textarea',
    '#description' => t('Enter your planning factors here'),
    '#default_value' => $farm_grazing_factors['value'],
  );

  $form['concerns']['factors_format'] = array(
    '#type' => 'value',
    '#value' => $farm_grazing_factors['format'],
  );

  if (!$grazing_growing_season) {
    $form['concerns']['non_growing'] = array(
      '#type' => 'fieldset',
      '#title' => t('Non-growing plan variables'),
      '#description' => t('Enter the follow parameters for the non-growing season plan.'),
      '#collapsible' => FALSE,
      '#collapsed' => FALSE,
    );
    $form['concerns']['non_growing']['days_bulk_feeding'] = array(
      '#type' => 'textfield',
      '#title' => t('Days of bulk feeding'),
      '#default_value' => $days_bulk_feeding,
      '#required' => TRUE,
    );
    $form['concerns']['non_growing']['days_of_drought_reserve'] = array(
      '#type' => 'textfield',
      '#title' => t('Days of drought reserve required'),
      '#default_value' => $days_of_drought_reserve,
      '#required' => TRUE,
    );
    $form['concerns']['non_growing']['expected_days_non_growth'] = array(
      '#type' => 'textfield',
      '#title' => t('Expected days of non-growth'),
      '#default_value' => $expected_days_non_growth,
      '#disabled' => TRUE,
    );
    $form['concerns']['non_growing']['expected_rotations'] = array(
      '#type' => 'textfield',
      '#title' => t('Number of planned rotations through paddocks'),
      '#default_value' => $expected_rotations,
      '#required' => TRUE,
    );
  }

  $form['concerns']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
    '#submit' => array('farm_grazing_plan_dashboard_form_submit'),
    '#validate' => array('farm_grazing_plan_dashboard_form_validate'),
  );

  // Add "Implement the plan" content.
  $form['implement'] = array(
    '#type' => 'fieldset',
    '#title' => t('Implementing the plan'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#weight' => 120,
  );
  $form['implement']['content'] = array(
    '#type' => 'markup',
    '#markup' => '<h4>' . t('Recheck your plan') . '</h4>' .
    '<p>' . t('Review your plan regularly, print out reports, check that the plan is on track and make adjustments as needed.') . '</p>' .
    '<h4>' . t('Monitor Your Paddocks') . '</h4>' .
    '<p>' . t('Monitor the growth and recovery of your paddocks. Also assess the quality of the paddock you are moving the herd to and record it as a monitor event here.') . '</p>' .
    '<ul>' .
    '<li><a href="' . base_path() . 'farm/assets/groups">' . t('Herd movement ') . '</a></li>' .
    '<ul>' .
    '<li>' . t('Click Herd movements link above') . '</li>' .
    '<li>' . t('Select a herd to move') . '</li>' .
    '<li>' . t('Click Add activity') . '</li>' .
    '<li>' . t('Click Movements') . '</li>' .
    '<li>' . t('Select paddock for "Movement to"') . '</li>' .
    '<li>' . t('Click "Save Log"') . '</li>' .
    '</ul>' .
    '<li><a href="' . base_path() . 'farm/area">' . t('Observations on paddock leaving') . '</a></li>' .
    '<ul><ul>' .
    '<li>' . t('Select paddock you are making an observation of') . '</li>' .
    '<li>' . t('Click "Add Observation"') . '</li>' .
    '<li>' . t('Use "General" for errors or general text comments') . '</li>'
    .
    '<li>' . t('Use "Quantity"->"Add another item" to add quantity observations') . '</li>' .
    '</ul></ul>' .
    '<ul>' .
    '<li>' . t('Residual feed left on the paddock') . '</li>' .
    '<li>' . t('The current growth rate on the paddock') . '</li>' .
    '<li>' . t('Was a serious error made on the paddock') . '</li>' .
    '<li>' . t('Soil moisture') . '</li>' .
    '</ul>' .
    '<li><a href="' . base_path() . 'farm/area">' . t('Observations on paddock entering') . '</a></li>' .
    '<ul><ul>' .
    '<li>' . t('Select paddock you are making an observation of') . '</li>' .
    '<li>' . t('Click "Add Observation"') . '</li>' .
    '<li>' . t('Use "General" for errors or general text comments') . '</li>' .
    '<li>' . t('Use "Quantity"->"Add another item" to add quantity observations') . '</li>' .
    '</ul></ul>' .
    '<ul>' .
    '<li>' . t('Quality of paddock') . '</li>' .
    '<li>' . t('Estimate of feed on paddock') . '</li>' .
    '<li>' . t('Soil moisture') . '</li>' .
    '</ul>' .
    '</ul>' .
    '<h4>' . t('How to Replan if Needed') . '</h4>' .
    '<p>' . t('You can make changes to your plan at any time. Actual events obviously can not be changed, but you can replan future events. There are two approaches to replanning.') . '<ol>' .
    '<li>' . t('is to just update the current plan for minor changes') . '</li>' .
    '<li>' . t('another is to create a new plan, for major changes, based on the remaining days and abandon the old plan for now.') . '</li>' .
    '<p>' . t('The advantage to making a new plan is that you can keep the original plan around for archival purposes and look at it in the future to re-assess assumptions or conditions that changed and forced the replan. On the new plan, you might need to sell stock to reduce the requirements for intake if you are in a drought or had a fire that burnt through some padocks or add more paddocks that you had not planned to use in the original plan. If you only need to make minor changes then use your existing plan, otherwise start a new plan.') . '</p>',
  );

  return $form;
}

/**
 * theme_farm_grazing_plan_dashboard_form
 *
 * theme handler for farm_grazing_plan_dashboard_form
 */
function theme_farm_grazing_plan_dashboard_form($variables) {
  $form = &$variables['form'];
  $plan = $form['plan']['#value'];
  // $growing_season = $form['growing_season']['#value'];.
  $output = drupal_render($form['plan']);
  $output .= drupal_render($form['spacer_1']);
  $output .= drupal_render($form['introduction']);
  $output .= drupal_render($form['section_1']['text']);
  $output .= drupal_render($form['section_2']['factors']);
  $output .= drupal_render($form['factors_format']);

  if (array_key_exists('non_growing', $form)) {
    $output .= drupal_render($form['non_growing']);
  }

  $output .= drupal_render_children($form);

  return $output;
}

/**
 * farm_grazing_plan_dashboard_form_validate
 *
 * validate handler for farm_grazing_plan_dashboard_form
 */
function farm_grazing_plan_dashboard_form_validate($form, &$form_state) {
  // Load the plan object into an EntityMetadataWrapper.
  $wrapper = entity_metadata_wrapper('farm_plan', $form_state['values']['plan']);

  // Only perform the following validation if this is a non-growing season.
  if (!empty($wrapper->field_grazing_growing_season->value())) {
    return;
  }

  if (!is_numeric($form_state['values']['days_bulk_feeding']) or
      $form_state['values']['days_bulk_feeding'] < 0) {
    form_set_error('days_bulk_feeding', t('Days bulk feeding must be numeric >= 0'));
  }
  if (!is_numeric($form_state['values']['days_of_drought_reserve']) or
      $form_state['values']['days_of_drought_reserve'] < 0) {
    form_set_error('days_of_drought_reserve', t('Days of drought reserve required must be numeric >= 0'));
  }
  if (!is_numeric($form_state['values']['expected_rotations']) or
      $form_state['values']['expected_rotations'] < 0) {
    form_set_error('expected_rotations', t('Number of planned rotations must be numeric >= 0'));
  }
}

/**
 * farm_grazing_plan_dashboard_form_submit
 *
 * submit handler for farm_grazing_plan_dashboard_form
 */
function farm_grazing_plan_dashboard_form_submit($form, &$form_state) {
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
