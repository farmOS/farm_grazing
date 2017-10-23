<?php

include_once "farm_grazing.utils.inc";

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

