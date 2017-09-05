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
    '#markup' => t('<p>You can create new plans, edit plan details and archive plans from this page.</p>'),
  );
  $form['container']['manage']['edit'] = farm_grazing_plan_form_details($plan, $form_state);

  // Getting started.
  $form['container']['start'] = array(
  '#type' => 'fieldset',
  '#title' => t('Getting started'),
  );

  $form['container']['start']['content'] = array(
    '#markup' => t('<h1>Getting Started</h1>

    <p>Welcome to the Holistic Planning Application.</p>

    <h1>For the First Time</h1>
    <p>To get started, you need to define your ranch\'s paddocks. This is done on the "Areas" tab at the top of the page. This needs to be done once initially and anytime you decide to change the layout of paddocks on the ranch.</p>

    <h1>Making a New Plan</h1>
    <p>If you have already defined your Paddocks for the ranch, then you need to create a Plan, use the [Manage Plan] tab on the left, or select an existing plan from the drop down menu. When you start the application it will automatically remember the plan you used last.</p>

    <p>Now that you have a "active" plan to work with, you can follow the planning process by using the tabs on the left in "Grazing Plan". The process follows the order of the tabs from top to bottom. You can revisit any tab as needed to review or change your inputs.</p>

    <p>The "Calendar" tab on the left will be an essential part of the planning application. Most elements of your plan will have dates associated with them and anything with a related date or date range will get rendered in the Calendar. This provides you with critical information like whether events conflict time-wise and may need to be moved around or other actions may need to be planned to accomdate the conflicts.</p>

    <p>Next Steps: Simple go to the next tab "Identify Planning Factors" and work your way down the planning list. Each tab has a context specific [Help] button to discuss issues and concerns related to that planning step.</p>'),
    );

  // Display tabs for growing season plans.
  if (!empty($plan->growing_season)) {

    // Identify planning factors.
    $form['container']['factors'] = array(
    '#type' => 'fieldset',
    '#title' => t('Identify planning factors'),
    );

    $form['container']['factors']['content'] = array(
      '#markup' => t('<h1>Identify Planning Factors</h1>
      <p>Gather the team of people that implemented last years plan and that will be responsible for this year\'s plan. Consider your goals and the goals for the landscape your wish to create. Make a list of factors that will touch on livestock, wildlife, crops, haying, etc. Think through how the entire ranch will be used concidering the number of cells, herds, stocking rates, cropping, drought reserve and biological monitoring.</p>
      <p>Enter the ideas in the planning factors field below or under the "Manage Plan" tab.</p>'),
    );

    // TODO: Get plan.factors and populate textarea
    //       Add Save button for planning factors.
    //       store factors in plan.factors in database.
    $form['container']['factors']['container']['factors'] = array(
      '#type' => 'textarea',
      '#title' => t('Enter or edit your factors in this list:'),
    );

    // Management concerns.
    $form['container']['concerns'] = array(
    '#type' => 'fieldset',
    '#title' => t('Management concerns'),
    );

    $form['container']['concerns']['content'] = array(
      '#markup' => t('<h1>Management Concerns</h1>
      <p>In this section, think about various factors that influence your calendar for the plan. Consider:</p>
      <ul>
          <li>Livestock events like birthing, breeding, weaning, etc and when these are planned.
          <li>Management events such as vacations, family or community events that need to be planned around.
          <li>Other specific events like hunting seasons that might influence the sequence of paddock moves.
      </ul>
      <p>Enter these events into the Calendar on the "Calendar" tab. You will come back to this later when you are planning your moves and it will be helpful to be able to see these events while planning moves.</p>
      <ul>
        <li>Go to the Calendar tab and enter these events.
        <li>Go to the Identify Planning Factors tab and add more factors that you want to remember.
      </ul>'),
    );

    // Define paddocks exclusions.
    $form['container']['exclusions'] = array(
    '#type' => 'fieldset',
    '#title' => t('Define paddock exclusions'),
    );

    $form['container']['exclusions']['content'] = array(
      '#markup' => t('<h1>Paddock Livestock Exclusions and Special Attention</h1>
      <p>Enter dates when a paddock can not under any circomstanceces have a herd on it and select type "Livestock". Also enter dates for paddocks that need special attention and select type "Special". Such paddocks could include a sacrificial paddock where you plan to run some animals on continuous graze, bare, eroding ground that needs healing, areas you want to rest for brush cover for wildlife, areas where you need to remove forage for fire protection, paddocks you plan to strip graze. Review the list of planning factors.</p>'),
    );
    $form['container']['exclusions']['form'] = farm_grazing_exclusions_form();

    // Rate paddock productivity.
    $form['container']['productivity'] = array(
    '#type' => 'fieldset',
    '#title' => t('Rate paddock productivity'),
    );

    $form['container']['productivity']['content'] = array(
      '#markup' => t('<h1>Rate Paddock Productivity</h1>
      <p>Enter a value for each paddock in the Forage Quality field, then click the [Save] button. See the Help button for more information.</p>
      <p>For Forage Quality, you should enter ADA/H for each paddock, or rate it 1-10.</p>      
      <p>Est. Relative Quality is the estimated animals days for the paddock based on Forage Quality if you entered a value in ADA/H, but if you entered an rating, then the number will be a relative ranking of the amount of forage each paddock can give relative to the other paddocks.</p>'),
    );

    // Determine recovery periods.
    $form['container']['recovery'] = array(
    '#type' => 'fieldset',
    '#title' => t('Determine recovery periods'),
    );

    $form['container']['recovery']['content'] = array(
      '#markup' => t('<h1>Determine recovery periods</h1>
      <p>Enter your minimum and maximum recovery periods (in days) for each planned month. Unless you have a great many paddocks per herd and can safely choose a single recovery period, you will have to determine the expected plant recovery period under fast growth conditions and the expected plant recovery period under slow growth conditions.</p>'),
    );

    // Define the herd(s).
    $form['container']['herds'] = array(
    '#type' => 'fieldset',
    '#title' => t('Define the herd(s)'),
    );

    $form['container']['herds']['content'] = array(
      '#markup' => t('<h1>Define the herd(s)</h1>      
      <p>A herd is defined as a group of animals that will be rotated between paddock to together regardless of the make up of animals in the herd.</p>      
      <p>Most smaller ranches will only have a single herd as you will need more paddocks to support multiple herds and allow for adequate recovery periods of the paddocks.</p>      
      <p>If you are new to this planning method or you have a smaller ranch, try to keep your plan simple to start with and built it based on a single herd.</p>'),
    );

    // Select paddocks for the active herd.
    $form['container']['herd_paddocks'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select paddocks for a herd'),
    );

    $form['container']['herd_paddocks']['content'] = array(
      '#markup' => t('<h1>Select paddocks for a herd</h1>
      <p>Select a herd if you have defined more than one</p>
      <p>Then select or unselect the paddocks you want to use for this herd. Paddocks may be selected more than once for a given herd or between multiple herds. Once a paddock has be selected for grazing it will show the recovery peroid for that paddock and it is not recommended that it be grazed again until after the recovery period.</p>'),
    );

    // Check historical grazing patterns.
    $form['container']['history'] = array(
    '#type' => 'fieldset',
    '#title' => t('Check historical grazing patterns'),
    );

    $form['container']['history']['content'] = array(
      '#markup' => t('<h1>Check historical grazing patterns</h1>
      <p> Skip this step if this is your first grazing plan. Otherwise, check paddock by paddock over previous grazing charts for evidence of inappropriate heavy use of individual paddocks, repeated early or late season use of particular paddocks, or of paddocks that failed to receive adequate recovery time in the recent past. If paddocks were marked as heavily grazed, especially early or late in the season last year, exclude them in the early and late growing season this year to avoid repetition.</p>
      <p>This table counts the frequency of historical use in the given months. Ideally one would like to have these numbers consistent across the months in the plan over the long term. In the short term avoid reuse at start or end of the growing season. Also if a paddock has been used a lot in a given month then avoid using that month also.</p>'),
    );

    // Check grazing periods.
    $form['container']['periods'] = array(
    '#type' => 'fieldset',
    '#title' => t('Check grazing periods'),
    );

    $form['container']['periods']['content'] = array(
      '#markup' => t('<h1>Check Grazing Periods</h1>
      <p>Check to see that recovery periods are adequate in paddocks with longer grazing periods. Use the Check Min/Max Recovery Period below and if any recovery period is much too short, you must add days to the minimum grazing periods in other paddocks that can absorb them. Follow the same procedure for maximum grazing periods, though the problems will probably be less critical if you can\'t make complete adjustments.</p>
      <h1>Actual Min/Max Grazing Periods</h1>'),
    );

    // Plot grazings on calendar.
    $form['container']['herd_paddock_order'] = array(
    '#type' => 'fieldset',
    '#title' => t('Select paddock order for grazing herd'),
    );

    $form['container']['herd_paddock_order']['content'] = array(
      '#markup' => t('<h1>Plot Grazings on Calendar</h1>'),
    );

    // Implement the plan.
    $form['container']['implement'] = array(
    '#type' => 'fieldset',
    '#title' => t('Implement the plan'),
    );

    $form['container']['implement']['content'] = array(
      '#markup' => t('<h1>Implement the plan</h1>
      <h2>Recheck your plan</h2>      
      <p>Review your plan regularly, print out reports, check that the plan is on track and make adjustments as needed.</p>
      <h2>Monitor Your Paddocks</h2>      
      <p>Monitor the growth and recovery of your paddocks. Also assess the quality of the paddock you are moving the herd to and record it as a monitor event here.</p>
      <h2>Record the Actual Moves</h2>      
      <p>When you make an actual move assess the residual feed left on the paddock and enter it into the actual move by clicking on the move event in the calendar. Note what kind of growth rate you had while occupying the paddock. If there was a serious error made in the paddock you are leaving check the box and add a description performance area.</p>      
      <p>Check the condition of the paddock that you are moving the herd into and add a monitor event using the button above.</p>
      <h2>How to Replan if Needed</h2>      
      <p>You can make changes to your plan at any time. Actual events obviously can not be changed, but you can replan future events. There are two approaches to replanning. 1) is to just update the current plan for minor changes, 2) another is to create a new plan, for major changes, based on the remaining days and abandon the old plan for now. The advantage to making a new plan is that you can keep the original plan around for archival purposes and look at it in the future to re-assess assumptions or conditions that changed and forced the replan. On the new plan, you might need to sell stock to reduce the requirements for intake if you are in a drought or had a fire that burnt through some padocks or add more paddocks that you had not planned to use in the original plan. If you only need to make minor changes then use your existing plan, otherwise start a new plan.</p>'),
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

/**
 * Exclusions list, add, edit form.
 */
function farm_grazing_exclusions_form() {

  // Load a list of exclusions from the database.
  $query = db_query('SELECT * FROM {farm_grazing_paddock_exclusions}');

  // Build a themed table of exclusions.
  $table = array(
    'header' => array(
      t('Paddock'),
      t('Exc. Start'),
      t('Exc. End'),
      t('Reason'),
      t('Edit'),
    ),
    'rows' => array(),
    'empty' => t('No exclusions found.')
  );
  $records = $query->fetchAll();
  foreach ($records as $record) {

    // If the paddock ID is not available, skip it.
    if (empty($record->paddock_id)) {
      continue;
    }

    // Load the paddock.
    $term = taxonomy_term_load($record->paddock_id);

    // Format the start and end dates.
    $format = 'Y-m-d';
    $start_date = date($format, $record->start_date);
    $end_date = date($format, $record->end_date);

    // Assemble the row.
    $table['rows'][] = array(
      $term->name,
      $start_date,
      $end_date,
      $record->reason,
    );
  }
  $form['table'] = array(
    '#markup' => theme('table', $table),
  );

  // Load all paddocks and generate an options list.
  $paddocks = farm_area_load_areas('paddock');
  $paddock_options = array();
  foreach ($paddocks as $paddock) {
    $paddock_options[$paddock->tid] = $paddock->name;
  }

  // Form for adding a new exclusion.
  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add an exclusion'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['paddock'] = array(
    '#type' => 'select',
    '#title' => t('Paddock'),
    '#description' => t('Select the paddock to create an exclusion for.'),
    '#options' => $paddock_options,
    '#required' => TRUE,
  );
  $form['add']['start_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('Start date'),
    '#description' => t('When does the exclusion begin?'),
    '#date_label_position' => 'within',
    '#date_format' => 'M j Y',
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#required' => TRUE,
  );
  $form['add']['end_date'] = array(
    '#type' => 'date_popup',
    '#title' => t('End date'),
    '#description' => t('When does the exclusion end?'),
    '#date_label_position' => 'within',
    '#date_format' => 'M j Y',
    '#date_type' => DATE_FORMAT_UNIX,
    '#date_year_range' => '-10:+3',
    '#required' => TRUE,
  );
  $form['add']['reason'] = array(
    '#type' => 'textfield',
    '#title' => t('Reason'),
    '#description' => t('Describe the reason for this exclusion.'),
    '#required' => TRUE,
  );
  $form['add']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save exclusion'),
    '#submit' => array('farm_grazing_exclusions_form_submit'),
  );

  // Return the form.
  return $form;
}

/**
 * Submit function for the exclusions form.
 */
function farm_grazing_exclusions_form_submit($form, &$form_state) {

  // Gather the values
  // ie: $form_state['values']['end_date']

  // Convert dates to timestamps.
  // ie: strtotime($form_state['values']['end_date'])

//  // Build a record to save to the database.
//  $record = array(
//    'id' => NULL,
//    'paddock_id' => $form_state['values']['paddock_id'],
//    'start_date' => '',
//    'end_date' => '',
//    'reason' => '',
//  );
//  drupal_write_record('farm_grazing_paddock_exclusions', $record);

}
