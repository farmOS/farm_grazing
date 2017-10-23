<?php

include_once "farm_grazing.utils.inc";

/**
 * Paddock summary page callback
 */
function farm_grazing_plan_summary_page($plan_obj) {
  
  // get the plan id from the plan object
  $plan = $plan_obj->id;

  // Set the page title
  drupal_set_title(t('Paddock Summary and Grazing Periods'));

  $table = array(
    'header' => array(
      t('Paddock'),
      t('Area'),
      t('Quality'),
      t('Act. Min. Grazing Days'),
      t('Act. Max. Grazing Days'),
    ),
    'rows' => array(),
    'empty' => t('No paddocks found.'),
  );

  $chk_min_recovery = -1;
  $chk_max_recovery = -1;
  $paddocks = getGrazingDaysForPlanId($plan, $chk_min_recovery, $chk_max_recovery, false);

  $table['rows'] = $paddocks;

  // Return markup
  $form['text'] = array(
    '#markup' => t('
      <p>Check to see that recovery periods are adequate in paddocks with longer grazing periods.</p>
      <h1>Actual Min/Max Grazing Periods</h1>
    '),
  );

  $form['table'] = array(
    '#markup' => theme('table', $table),
  );

  $form['summary']['check_min'] = array(
    '#prefix' => '<p>',
    '#markup' => t('Check Min. Recovery Period: ') . round($chk_min_recovery),
    '#suffix' => '</p>',
  );

  $form['summary']['check_max'] = array(
    '#prefix' => '<p>',
    '#markup' => t('Check Max. Recovery Period: ') . round($chk_max_recovery),
    '#suffix' => '</p>',
  );

  return $form;
}

