<?php

include_once "farm_grazing.utils.inc";

/**
 * Management concerns page callback.
 */
function farm_grazing_plan_concerns_page($plan_obj) {

  $plan = $plan_obj->id;

  // Set the page title.
  drupal_set_title(t('Management concerns'));

  // Return markup.
  return array(
    '#markup' => t('
      <p>In this section, think about various factors that influence your calendar for the plan. Consider:</p>
      <ul>
          <li>Livestock events like birthing, breeding, weaning, etc and when these are planned.
          <li>Management events such as vacations, family or community events that need to be planned around.
          <li>Other specific events like hunting seasons, parasites, or flooding that might influence the sequence of paddock moves.
      </ul>
      <p>Enter management events into the Calendar on the "Calendar" tab. Enter other events into your ') . '<a href="' . base_path() . 'farm/plan/' . $plan . '/edit">' . t('Planning Factors') . '</a>' . t(' for the plan for now.</p>
      <p>You will come back to this later when you need to enter paddock exclusions and are planning your moves as it will be helpful to be able to see these events while planning moves.</p>
    '),
  );
}

