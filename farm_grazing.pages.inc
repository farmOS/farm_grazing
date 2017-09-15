<?php

/**
 * @file
 * Grazing pages code.
 */

  //'page callback' => 'farm_grazing_plan_concerns_page',
  //'page callback' => 'farm_grazing_plan_paddocks_select_page',
  //'page callback' => 'drupal_get_form',
  //'page callback' => 'farm_grazing_plan_productivity_page',
  //'page callback' => 'farm_grazing_plan_recovery_page',
  //'page callback' => 'farm_grazing_plan_summary_page',
  //'page callback' => 'farm_grazing_plan_herds_page',
  //'page callback' => 'farm_grazing_plan_herds_paddocks_page',
  //'page callback' => 'farm_grazing_plan_history_page',
  //'page callback' => 'farm_grazing_plan_rotations_page',
  //'page callback' => 'farm_grazing_plan_implement_page',
  //'page callback' => 'farm_grazing_plan_implement_status_page',


/**
 * Management concerns page callback.
 */
function farm_grazing_plan_concerns_page($plan) {

  // Set the page title.
  drupal_set_title(t('Management concerns'));

  // Return markup.
  return array(
    '#markup' => t('
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
}

/**
 * Paddocks selection page callback
 */
function farm_grazing_plan_paddocks_select_page($plan) {
  
  // Set the page title
  drupal_set_title(t('Select paddocks to consider for plan'));

  // Return markup
  return array(
    '#markup' => t('
      <p>In this section select the paddocks that you want to be considered in this plan. You can ignore paddocks that for whatever reason you don\'t want considered at all, but you can include paddocks that might have partial exclusions.<p>
      <p><b>TODO:</b> Make a list of paddocks with checkboxes to select.</p>
    '),
  );
}

/**
 * Paddock exclusions list, add, edit form.
 */
function farm_grazing_exclusions_form($form, &$form_state) {

  // Set the page title.
  drupal_set_title(t('Paddock exclusions'));

  // Load a list of exclusions from the database.
  $query = db_query('SELECT * FROM {farm_grazing_paddock_exclusions}');

  // Build a themed table of exclusions.
  $table = array(
    'header' => array(
      t('Paddock'),
      t('Exc. Start'),
      t('Exc. End'),
      t('Reason'),
    ),
    'rows' => array(),
    'empty' => t('No exclusions found.'),
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
  $form['add']['paddock_id'] = array(
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
 * Submit function for the paddock exclusions form.
 */
function farm_grazing_exclusions_form_submit($form, &$form_state) {

  // Build a record and save it to the database.
  $record = array(
    'id' => NULL,
    'paddock_id' => $form_state['values']['paddock_id'],
    'start_date' => strtotime($form_state['values']['start_date']),
    'end_date' => strtotime($form_state['values']['end_date']),
    'reason' => $form_state['values']['reason'],
  );
  drupal_write_record('farm_grazing_paddock_exclusions', $record);
}

/**
 * Set paddock productivity page callback
 */
function farm_grazing_plan_productivity_page($plan) {
  
  // Set the page title
  drupal_set_title(t('Rate Paddock Productivity'));

  // Return markup
  return array(
    '#markup' => t('
      <p>Enter a value for each paddock in the Forage Quality field, then click the [Save] button. See the Help button for more information.</p>
      <p>For Forage Quality, you should enter ADA/H for each paddock, or rate it 1-10.</p>
      <p>Est. Relative Quality is the estimated animals days for the paddock based on Forage Quality if you entered a value in ADA/H, but if you entered an rating, then the number will be a relative ranking of the amount of forage each paddock can give relative to the other paddocks.</p>'),
  );
}

/**
 * Paddock recovery page callback.
 */
function farm_grazing_plan_recovery_page($plan) {

  // Set the page title.
  drupal_set_title(t('Determine recovery periods'));

  // Return markup.
  return array(
    '#markup' => t('
      <p>Enter your minimum and maximum recovery periods (in days) for each planned month. Unless you have a great many paddocks per herd and can safely choose a single recovery period, you will have to determine the expected plant recovery period under fast growth conditions and the expected plant recovery period under slow growth conditions.</p>'),
  );
}

/**
 * Paddock summary page callback
 */
function farm_grazing_plan_summary_page($plan) {
  
  // Set the page title
  drupal_set_title(t('Paddock Summary and Grazing Periods'));

  // Return markup
  return array(
    '#markup' => t('
      <p>Check to see that recovery periods are adequate in paddocks with longer grazing periods. Use the Check Min/Max Recovery Period below and if any recovery period is much too short, you must add days to the minimum grazing periods in other paddocks that can absorb them. Follow the same procedure for maximum grazing periods, though the problems will probably be less critical if you can\'t make complete adjustments.</p>
      <h1>Actual Min/Max Grazing Periods</h1>
      <p><b>TODO</b></p>
    '),
  );
}

/**
 * Herd page callback.
 */
function farm_grazing_plan_herds_page($plan) {

  // Set the page title.
  drupal_set_title(t('Manage Herds (Add/Combine/Split/Ship)'));

  // Return markup.
  return array(
    '#markup' => t('
      <p>A herd is defined as a group of animals that will be rotated between paddocks to together regardless of the make up of animals in the herd.</p>
      <p>Herds are defined in farmOS as groups under the "Assets -> Animals" menu, then they can be selected here. You can also add to a herd, combine herds, split some animals out of a herd, or ship animals out of the plan.</p>
      <p><b>TODO:</b> List herds</p>
      <p><b>TODO:</b> Add tools to add/combine/split/ship animals or herd</p>
    '),
  );
}

/**
 * Paddocks page callback.
 */
function farm_grazing_plan_herds_paddocks_page($plan) {

  // Set the page title.
  drupal_set_title(t('Select paddocks for a herd'));

  // Return markup.
  return array(
    '#markup' => t('
      <p>Select a herd if you have defined more than one</p>
      <p>Then select or unselect the paddocks you want to use for this herd. Paddocks may be selected more than once for a given herd or between multiple herds. Once a paddock has be selected for grazing it will show the recovery peroid for that paddock and it is not recommended that it be grazed again until after the recovery period.</p>'),
  );
}

/**
 * History page callback.
 */
function farm_grazing_plan_history_page($plan) {

  // Set the page title.
  drupal_set_title(t('Check historical grazing patterns'));

  // Return markup.
  return array(
    '#markup' => t('
      <p> Skip this step if this is your first grazing plan. Otherwise, check paddock by paddock over previous grazing charts for evidence of inappropriate heavy use of individual paddocks, repeated early or late season use of particular paddocks, or of paddocks that failed to receive adequate recovery time in the recent past. If paddocks were marked as heavily grazed, especially early or late in the season last year, exclude them in the early and late growing season this year to avoid repetition.</p>
      <p>This table counts the frequency of historical use in the given months. Ideally one would like to have these numbers consistent across the months in the plan over the long term. In the short term avoid reuse at start or end of the growing season. Also if a paddock has been used a lot in a given month then avoid using that month also.</p>'),
  );
}

/**
 * Periods page callback.
 */
function farm_grazing_plan_rotations_page($plan) {

  // Set the page title.
  drupal_set_title(t('Select paddock rotation order for each herd'));

  // Return markup.
  return array(
    '#markup' => '
      <p><b>TODO</b></p>
    '
  );
}

/**
 * Implement page callback.
 */
function farm_grazing_plan_implement_page($plan) {

  // Set the page title.
  drupal_set_title(t('Implement the plan'));

  // Return markup.
  return array(
    '#markup' => t('
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
}

/**
 *
 */
function farm_grazing_plan_implement_status_page($plan) {

  // Set the page title
  drupal_set_title(t('Plan Status'));

  // Return markup
  return array(
    '#markup' => t('
      <p><b>TODO:</b> generate plan status report.</p>
    '),
  );
}
