<?php

/**
 * @file
 * Farm constraint hook implementations.
 */

/**
 * Implements hook_farm_constraint().
 */
function farm_grazing_farm_constraint($type, $bundle, $id) {

  // Define grazing plan references to check for.
  $plan_references = array(
    'herd' => array(
      'type' => 'farm_asset',
      'bundles' => 'group',
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
        'farm_grazing_paddock_exclusions' => 'paddock_id',
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
  foreach ($plan_references as $info) {
    if ($type == $info['type'] && $bundle == $info['bundle']) {
      foreach ($info['tables'] as $table => $column) {
        $exists = db_query('SELECT COUNT(*) FROM {' . $table . '} WHERE ' . $column . ' = :id', array(':id' => $id))->fetchField();
        if (!empty($exists)) {
          return TRUE;
        }
      }
    }
  }

  // If nothing was found, return FALSE.
  return FALSE;
}
