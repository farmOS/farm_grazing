<?php

/**
 * @file
 * Grazing pages code.
 */

include_once "farm_grazing.utils.inc";



// -------------- config forms ---------------------

/**
 * function farm_grazing_config_animal_types_form
 *
 * TODO I think Mike is doing this elsewhere??
 */
function farm_grazing_config_animal_types_form($form, $form_state) {

}


//-------------- Start of page/form callbacks --------------------

//'page callback' => 'drupal_get_form(farm_grazing_select_paddocks_form)',
include "farm_grazing.paddocks.select.inc";

//'page callback' => 'drupal_get_form(farm_grazing_exclusions_form)',
include "farm_grazing.paddocks.exclusions.inc";

//'page callback' => 'drupal_get_form(farm_grazing_plan_productivity_form'),
include "farm_grazing.paddocks.productivity.inc";

//'page callback' => 'drupal_get_form(farm_grazing_plan_recovery_form)',
include "farm_grazing.paddocks.recovery.inc";

//'page callback' => 'farm_grazing_plan_summary_page',
include "farm_grazing.paddocks.summary.inc";

//'page callback' => 'drupal_get_form(farm_grazing_plan_herds_form)',
include "farm_grazing.herds.manage.inc";

//'page callback' => 'drupal_get_form(farm_grazing_plan_history_form)',
include "farm_grazing.herds.history.inc";

//'page callback' => 'drupal_get_form(farm_grazing_plan_rotations_form)',
include "farm_grazing.herds.rotations.inc";

//'page callback' => 'farm_grazing_plan_plot',
include "farm_grazing.herds.rotations.plot.inc";
