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

