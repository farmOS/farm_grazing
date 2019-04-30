<?php

/**
 * @file
 * Farm constraint hook implementations.
 */

/**
 * Implements hook_farm_constraint().
 */
function farm_grazing_farm_constraint($type, $bundle, $id) {

  // Define plan references to check for
  // (see farm_constraint_table_references_exist()).
  $plan_references = array(
    'herd' => array(
      'type' => 'farm_asset',
      'bundle' => 'group',
      'tables' => array(
        'farm_grazing_rotations' => 'herd_id',
        'farm_grazing_herds' => 'herd_id',
      ),
    ),
    'paddock' => array(
      'type' => 'taxonomy_term',
      'bundle' => 'farm_areas',
      'tables' => array(
        'farm_grazing_rotations' => 'paddock_id',
        'farm_grazing_plan_paddock' => 'paddock_id',
      ),
    ),
    'rotation' => array(
      'type' => 'log',
      'bundle' => 'farm_activity',
      'tables' => array(
        'farm_grazing_rotations' => 'log_id',
      ),
    ),
  );

  // Check to see if references exist.
  return farm_constraint_table_references_exist($plan_references, $type, $bundle, $id);
}
