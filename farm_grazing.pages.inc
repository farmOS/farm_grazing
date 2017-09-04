<?php

/**
 * @file
 * Grazing pages code.
 */

/**
 * Form for selecting plan.
 */
function farm_grazing_plan_form($form, &$form_state) {

  // Three scenarios:
  // 1) No plans exist. No default plan is set. No dropdown option selected.
  // 2) An existing plan was selected from the dropdown. Make it the default.
  // 3) The default plan is loaded automatically.

  // If a dropdown option was selected, make it the new default.
  if (!empty($form_state['values']['plan']) && is_numeric($form_state['values']['plan'])) {
    variable_set('farm_grazing_default_plan', $form_state['values']['plan']);
  }

  // Load the default plan ID (or NULL if not available).
  $default_plan = variable_get('farm_grazing_default_plan', NULL);

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
    '#default_value' => $default_plan,
  );

  // Create a container element. This will either contain a set of vertical
  // tabs for editing the plan, or a message. Wrap it with a div that will be
  // replaced if the plan select list above is changed.
  $form['container'] = array(
    '#prefix' => '<div id="grazing-plan">',
    '#suffix' => '</div>',
  );

  // If no default plan is set, display a message.
  if (empty($default_plan)) {

    // Message.
    $form['container']['#markup'] = t('Please select a plan above, or !create.', array('!create' => l(t('create a new plan'), 'farm/grazing/plan/create')));

    // Return the form, we're done.
    return $form;
  }

  // Load the default plan.
  if (is_numeric($default_plan)) {
    $query = db_query('SELECT * FROM {farm_grazing_plan} WHERE id = :id', array(':id' => $default_plan));
    $plan = $query->fetch();
  }

  // If the plan didn't load, bail.
  if (empty($plan)) {
    return $form;
  }

  // Make the container into a set of vertical tabs.
  $form['container']['#type'] = 'vertical_tabs';

  // Mange Plans.
  $form['container']['manage'] = array(
    '#type' => 'fieldset',
    '#title' => t('Manage Plan'),
  );
  $form['container']['manage']['content'] = array(
    '#markup' => '<p>You can create new plans, edit plan details and archive plans from this page.</p>',
  );
  $form['container']['manage']['edit'] = farm_grazing_plan_form_details($plan, $form_state);

  // Getting started.
  $form['container']['start'] = array(
  '#type' => 'fieldset',
  '#title' => t('Getting started'),
  );

  $form['container']['start']['content'] = array(
    '#markup' => '<h1>Getting Started</h1>

    <p>Welcome to the Holistic Planning Application.</p>

    <h1>For the First Time</h1>
    <p>To get started, you need to define your ranch\'s paddocks. This is done on the "Areas" tab at the top of the page. This needs to be done once initially and anytime you decide to change the layout of paddocks on the ranch.</p>

    <h1>Making a New Plan</h1>
    <p>If you have already defined your Paddocks for the ranch, then you need to create a Plan, use the [New Plan] button above, or select an existing plan from the drop down menu. When you start the application it will automatically select the last plan created.</p>

    <p>Now that you have a plan to work with, you can follow the planning process by using the tabs on the left of the "Planning" tab. The process follows the order of the tabs from top to bottom. You can revisit any tab as needed to review or change your inputs. Click the "Completed" checkbox to indicate that you have competed that tab. This is just a simple bookkeeping feature so you can easily remember what still needs to be worked out. You can still go back to a "completed" tab as needed and make changes or toggle the checkbox on/off.</p>

    <p>The "Calendar" tab above will be an essential part of the planning application. Most elements of your plan will have dates associated with them and anything with a related date or date range will get rendered in the Calendar. This provides you with critical information like whether events conflict time-wise and may need to be moved around or other actions may need to be planned to accomdate the conflicts.</p>

    <p>Next Steps: Simple go to the next tab "Identify Planning Factors" and work your way down the planning list. Each tab has a context specific [Help] button to discuss issues and concerns related to that planning step.</p>',
    );

  // Display tabs for growing season plans.
  if (!empty($plan->growing_season)) {

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
  // Display tabs for non-growing season plans
  } else {
  }

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

  // Return the form.
  return $form;
}

/**
 * Provides fields for adding/editing a plan's details.
 *
 * @param $plan
 *   Optionally provide an existing plan, to provide default values.
 * @param $form_state
 *   Optionally provide the form state from the parent form. If plan details
 *   are present in the form state input, they will be unset. This ensures that
 *   reloading this form via Drupal's #ajax will reload the new plan details.
 *
 * @return array
 *   Returns a form array.
 */
function farm_grazing_plan_form_details($plan = NULL, &$form_state = array()) {

  // If plan details are present in the form state input, unset them.
  if (!empty($form_state['input'])) {
    unset($form_state['input']['plan_name']);
    unset($form_state['input']['plan_start_date']);
    unset($form_state['input']['plan_end_date']);
    unset($form_state['input']['plan_growing_season']);
  }

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

  // Submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save plan'),
    '#submit' => array('farm_grazing_plan_form_details_submit'),
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

  // If a default plan is not already set, make this one the default.
  if (empty(variable_get('farm_grazing_default_plan', NULL))) {
    variable_set('farm_grazing_default_plan', $plan['id']);
  }

  // Redirect to /farm/grazing.
  $form_state['redirect'] = 'farm/grazing';
}

/**
 * Load plan options.
 */
function farm_grazing_plan_options() {

  // Start with an empty array.
  $plan_options = array();

  // Query the database for plans.
  $query = db_query('SELECT id, name, start_date, end_date FROM {farm_grazing_plan} ORDER BY end_date DESC');

  // Build a list of plan options.
  $date_format = 'M j Y';
  $records = $query->fetchAll();
  foreach ($records as $record) {
    $plan_options[$record->id] = $record->name . ' (' . date($date_format, $record->start_date) . ' - ' . date($date_format, $record->end_date) . ')';
  }

  // Return the options array.
  return $plan_options;
}
