<?php
/**
 * @file
 * farm_grazing.features.field_instance.inc
 */

/**
 * Implements hook_field_default_field_instances().
 */
function farm_grazing_field_default_field_instances() {
  $field_instances = array();

  // Exported field_instance: 'farm_plan-grazing-field_farm_date_range'.
  $field_instances['farm_plan-grazing-field_farm_date_range'] = array(
    'bundle' => 'grazing',
    'deleted' => 0,
    'description' => 'Define the date range for this grazing plan.',
    'display' => array(
      'default' => array(
        'label' => 'inline',
        'module' => 'date',
        'settings' => array(
          'format_type' => 'short',
          'fromto' => 'both',
          'multiple_from' => '',
          'multiple_number' => '',
          'multiple_to' => '',
          'show_remaining_days' => 1,
        ),
        'type' => 'date_default',
        'weight' => 0,
      ),
    ),
    'entity_type' => 'farm_plan',
    'field_name' => 'field_farm_date_range',
    'label' => 'Date range',
    'required' => 1,
    'settings' => array(
      'default_value' => 'now',
      'default_value2' => 'same',
      'default_value_code' => '',
      'default_value_code2' => '',
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 1,
      'module' => 'date',
      'settings' => array(
        'increment' => 15,
        'input_format' => 'm/d/Y - H:i:s',
        'input_format_custom' => '',
        'label_position' => 'none',
        'no_fieldset' => 0,
        'text_parts' => array(),
        'year_range' => '-10:+3',
      ),
      'type' => 'date_popup',
      'weight' => 1,
    ),
  );

  // Exported field_instance: 'farm_plan-grazing-field_farm_grazing_factors'.
  $field_instances['farm_plan-grazing-field_farm_grazing_factors'] = array(
    'bundle' => 'grazing',
    'default_value' => NULL,
    'deleted' => 0,
    'description' => 'Gather the team of people that implemented last years plan and that will be responsible for this year\'s plan. Consider your goals and the goals for the landscape your wish to create. Make a list of factors that will touch on livestock, wildlife, crops, haying, etc. Think through how the entire ranch will be used considering the number of cells, herds, stocking rates, cropping, drought reserve and biological monitoring. Enter all ideas in this planning factors field for future reference.',
    'display' => array(
      'default' => array(
        'label' => 'above',
        'module' => 'text',
        'settings' => array(),
        'type' => 'text_default',
        'weight' => 2,
      ),
    ),
    'entity_type' => 'farm_plan',
    'field_name' => 'field_farm_grazing_factors',
    'label' => 'Planning factors',
    'required' => 0,
    'settings' => array(
      'text_processing' => 1,
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 1,
      'module' => 'text',
      'settings' => array(
        'rows' => 5,
      ),
      'type' => 'text_textarea',
      'weight' => 3,
    ),
  );

  // Exported field_instance: 'farm_plan-grazing-field_grazing_growing_season'.
  $field_instances['farm_plan-grazing-field_grazing_growing_season'] = array(
    'bundle' => 'grazing',
    'default_value' => array(
      0 => array(
        'value' => 0,
      ),
    ),
    'deleted' => 0,
    'description' => 'Does this plan take place during the growing season? This will affect some calculations and recommendations when you are planning your rotations.',
    'display' => array(
      'default' => array(
        'label' => 'inline',
        'module' => 'list',
        'settings' => array(),
        'type' => 'list_default',
        'weight' => 1,
      ),
    ),
    'entity_type' => 'farm_plan',
    'field_name' => 'field_grazing_growing_season',
    'label' => 'Growing season',
    'required' => 0,
    'settings' => array(
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 1,
      'module' => 'options',
      'settings' => array(
        'display_label' => 1,
      ),
      'type' => 'options_onoff',
      'weight' => 2,
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('Date range');
  t('Define the date range for this grazing plan.');
  t('Does this plan take place during the growing season? This will affect some calculations and recommendations when you are planning your rotations.');
  t('Gather the team of people that implemented last years plan and that will be responsible for this year\'s plan. Consider your goals and the goals for the landscape your wish to create. Make a list of factors that will touch on livestock, wildlife, crops, haying, etc. Think through how the entire ranch will be used considering the number of cells, herds, stocking rates, cropping, drought reserve and biological monitoring. Enter all ideas in this planning factors field for future reference.');
  t('Growing season');
  t('Planning factors');

  return $field_instances;
}
