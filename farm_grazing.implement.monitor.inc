<?php

include_once "farm_grazing.utils.inc";

/**
 * Implement page callback.
 */
function farm_grazing_plan_implement_page($plan) {

  // Set the page title.
  drupal_set_title(t('Implement the plan'));

  // Return markup.
  return array(
    '#markup' => '<h2>' .t('Recheck your plan') . '</h2>' .
      '<p>' . t('Review your plan regularly, print out reports, check that the plan is on track and make adjustments as needed.') . '</p>' .
      '<h2>' . t('Monitor Your Paddocks') . '</h2>' .
      '<p>' . t('Monitor the growth and recovery of your paddocks. Also assess the quality of the paddock you are moving the herd to and record it as a monitor event here.') . '</p>' .
      '<ul>' .
      '<li><a href="' . base_path() . 'farm/assets/groups">' . t('Herd movements') . '</a></li>' .
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
      '<li>' . t('Use "General" for errors or general text comments') . '</li>' .
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
      '<h2>' . t('How to Replan if Needed') . '</h2>' .
      '<p>' . t('You can make changes to your plan at any time. Actual events obviously can not be changed, but you can replan future events. There are two approaches to replanning.') . '<ol>' .
      '<li>' . t('is to just update the current plan for minor changes') . '</li>' .
      '<li>' . t('another is to create a new plan, for major changes, based on the remaining days and abandon the old plan for now.') . '</li>' .
      '<p>' . t('The advantage to making a new plan is that you can keep the original plan around for archival purposes and look at it in the future to re-assess assumptions or conditions that changed and forced the replan. On the new plan, you might need to sell stock to reduce the requirements for intake if you are in a drought or had a fire that burnt through some padocks or add more paddocks that you had not planned to use in the original plan. If you only need to make minor changes then use your existing plan, otherwise start a new plan.') . '</p>',
  );
}

