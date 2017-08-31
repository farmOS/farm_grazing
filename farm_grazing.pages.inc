<?php

/**
 * @file
 * Grazing pages code.
 */

/**
 * Form for selecting plan.
 */
function farm_grazing_plan_form($form, &$form_state) {

  // Plan select list.
  $form['plan'] = array(
    '#type' => 'select',
    '#title' => t('Select plan'),
    '#options' => farm_grazing_plan_options(),
    '#required' => TRUE,
    '#ajax' => array(
      'wrapper' => 'grazing-plan',
      'callback' => 'farm_grazing_plan_form_ajax',
    ),
  );

  // Create a container element. This will either contain a set of vertical
  // tabs for editing the plan, or a message. Wrap it with a div that will be
  // replaced if the plan select list above is changed.
  $form['container'] = array(
    '#prefix' => '<div id="grazing-plan">',
    '#suffix' => '</div>',
  );

  // If no plan is selected, display a message.
  if (empty($form_state['values']['plan'])) {

    // Message.
    $form['container']['#markup'] = t('Please select a plan above, or !create.', array('!create' => l(t('create a new plan'), 'farm/grazing/plan/create')));

    // Return the form, we're done.
    return $form;
  }

  // If a plan was selected, load it.
  if (is_numeric($form_state['values']['plan'])) {
    $query = db_query('SELECT * FROM {farm_grazing_plan} WHERE id = :id', array(':id' => $form_state['values']['plan']));
    $plan = $query->fetch();
  }

  // If the plan didn't load, bail.
  if (empty($plan)) {
    return $form;
  }

  // Make the container into a set of vertical tabs.
  $form['container']['#type'] = 'vertical_tabs';

  // Getting started.
  $form['container']['start'] = array(
    '#type' => 'fieldset',
    '#title' => t('Getting started'),
  );

  // Identify planning factors.
  $form['container']['factors'] = array(
    '#type' => 'fieldset',
    '#title' => t('Identify planning factors'),
  );

  // Management concerns.
  $form['container']['concerns'] = array(
    '#type' => 'fieldset',
    '#title' => t('Management concerns'),
  );

  // Select paddocks and exclusions.
  $form['container']['paddocks'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select paddocks and exclusions'),
  );

  // Define the herd(s).
  $form['container']['herds'] = array(
    '#type' => 'fieldset',
    '#title' => t('Define the herd(s)'),
  );

  // Check historical grazing patterns.
  $form['container']['history'] = array(
    '#type' => 'fieldset',
    '#title' => t('Check historical grazing patterns'),
  );

  // Rate paddock productivity.
  $form['container']['productivity'] = array(
    '#type' => 'fieldset',
    '#title' => t('Rate paddock productivity'),
  );

  // Determine recovery periods.
  $form['container']['recovery'] = array(
    '#type' => 'fieldset',
    '#title' => t('Determine recovery periods'),
  );

  // Check grazing periods.
  $form['container']['periods'] = array(
    '#type' => 'fieldset',
    '#title' => t('Check grazing periods'),
  );

  // Plot grazings on calendar.
  $form['container']['calendar'] = array(
    '#type' => 'fieldset',
    '#title' => t('Plot grazings on calendar'),
  );

  // Implement the plan.
  $form['container']['implement'] = array(
    '#type' => 'fieldset',
    '#title' => t('Implement the plan'),
  );

  // Plan details.
  $form['container']['details'] = array(
    '#type' => 'fieldset',
    '#title' => t('Plan details'),
  );
  $form['container']['details']['edit'] = farm_grazing_plan_form_details($plan);

  // Add the plan form details submit function.
  $form['#submit'][] = 'farm_grazing_plan_form_details_submit';

  // Return the form.
  return $form;
}

/**
 * Ajax callback for the grazing plan form.
 */
function farm_grazing_plan_form_ajax($form, $form_state) {
  return $form['container'];
}

/**
 * Form for creating a new plan.
 */
function farm_grazing_plan_create_form($form, &$form_state) {

  // Load the plan details form elements and wrap them in a fieldset with
  // title and description.
  $form = farm_grazing_plan_form_details();
  $form['#type'] = 'fieldset';
  $form['#title'] = t('Plan details');
  $form['#description'] = t('Use the fields below to define a new grazing plan.');

  // Submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save plan'),
  );

  // Add the plan form details submit function.
  $form['#submit'][] = 'farm_grazing_plan_form_details_submit';

  // Return the form.
  return $form;
}

/**
 * Provides fields for adding/editing a plan's details.
 *
 * @param $plan
 *   Optionally provide an existing plan, to provide default values.
 *
 * @return array
 *   Returns a form array.
 */
function farm_grazing_plan_form_details($plan = NULL) {

  // Plan ID.
  $form['plan_id'] = array(
    '#type' => 'hidden',
    '#value' => !empty($plan->id) ? $plan->id : NULL,
  );

  // Plan name.
  $form['plan_name'] = array(
    '#type' => 'textfield',
    '#title' => t('Plan name'),
    '#maxlength' => 255,
    '#required' => TRUE,
    '#default_value' => !empty($plan->name) ? $plan->name : '',
  );

  // Start and end dates.
  $form['plan_start_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start date'),
    '#date_label_position' => 'within',
    '#date_format' => 'M j Y',
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#required' => TRUE,
    '#default_value' => !empty($plan->start_date) ? date('Y-m-d', $plan->start_date) : '',
  );
  $form['plan_end_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('End date'),
    '#date_label_position' => 'within',
    '#date_format' => 'M j Y',
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#required' => TRUE,
    '#default_value' => !empty($plan->end_date) ? date('Y-m-d', $plan->end_date) : '',
  );

  // Growing season?
  $form['plan_growing_season'] = array(
    '#type' => 'checkbox',
    '#title' => t('Growing season'),
    '#description' => t('Does this plan take place during the growing season?'),
    '#default_value' => !empty($plan->growing_season) ? TRUE : FALSE,
  );

  // Return the form array.
  return $form;
}

/**
 * Submit function for plan details. This needs to be manually added to any
 * forms that include the fields.
 */
function farm_grazing_plan_form_details_submit($form, &$form_state) {

  // Save the plan.
  $plan = array(
    'id' => $form_state['values']['plan_id'],
    'name' => $form_state['values']['plan_name'],
    'start_date' => strtotime($form_state['values']['plan_start_date']),
    'end_date' => strtotime($form_state['values']['plan_end_date']),
    'growing_season' => !empty($form_state['values']['plan_growing_season']) ? TRUE : FALSE,
  );
  $key = !empty($plan['id']) ? array('id') : array();
  drupal_write_record('farm_grazing_plan', $plan, $key);

  // Redirect to /farm/grazing.
  $form_state['redirect'] = 'farm/grazing';
}

/**
 * Load plan options.
 */
function farm_grazing_plan_options() {
  return array();
}
