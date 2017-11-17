<?php

/**
 * @file
 * farm_grazing.features.inc
 */

/**
 * Implements hook_ctools_plugin_api().
 */
function farm_grazing_ctools_plugin_api($module = NULL, $api = NULL) {
  if ($module == "field_group" && $api == "field_group") {
    return array("version" => "1");
  }
}

/**
 * Implements hook_views_api().
 */
function farm_grazing_views_api($module = NULL, $api = NULL) {
  return array("api" => "3.0");
}

/**
 * Implements hook_default_farm_plan_type().
 */
function farm_grazing_default_farm_plan_type() {
  $items = array();
  $items['grazing'] = entity_import('farm_plan_type', '{
    "type" : "grazing",
    "label" : "Grazing Plan",
    "weight" : 0,
    "rdf_mapping" : []
  }');
  return $items;
}
