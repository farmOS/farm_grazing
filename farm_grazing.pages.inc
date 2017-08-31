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
    $form['container']['#markup'] = t('Please select a plan above.');

    // Return the form, we're done.
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
 * Load plan options.
 */
function farm_grazing_plan_options() {
  return array();
}
