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

  // Exported field_instance: 'farm_plan-grazing-field_days_bulk_feeding'.
  $field_instances['farm_plan-grazing-field_days_bulk_feeding'] = array(
    'bundle' => 'grazing',
    'default_value' => NULL,
    'deleted' => 0,
    'description' => '',
    'display' => array(
      'default' => array(
        'label' => 'inline',
        'module' => 'number',
        'settings' => array(
          'decimal_separator' => '.',
          'prefix_suffix' => TRUE,
          'scale' => 0,
          'thousand_separator' => '',
        ),
        'type' => 'number_integer',
        'weight' => 2,
      ),
    ),
    'entity_type' => 'farm_plan',
    'field_name' => 'field_days_bulk_feeding',
    'label' => 'Days bulk feeding',
    'required' => 0,
    'settings' => array(
      'max' => '',
      'min' => 0,
      'prefix' => '',
      'suffix' => '',
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 0,
      'module' => 'number',
      'settings' => array(),
      'type' => 'number',
      'weight' => 4,
    ),
  );

  // Exported field_instance: 'farm_plan-grazing-field_days_of_drought_reserve'.
  $field_instances['farm_plan-grazing-field_days_of_drought_reserve'] = array(
    'bundle' => 'grazing',
    'default_value' => NULL,
    'deleted' => 0,
    'description' => '',
    'display' => array(
      'default' => array(
        'label' => 'inline',
        'module' => 'number',
        'settings' => array(
          'decimal_separator' => '.',
          'prefix_suffix' => TRUE,
          'scale' => 0,
          'thousand_separator' => '',
        ),
        'type' => 'number_integer',
        'weight' => 3,
      ),
    ),
    'entity_type' => 'farm_plan',
    'field_name' => 'field_days_of_drought_reserve',
    'label' => 'Days of drought reserve',
    'required' => 0,
    'settings' => array(
      'max' => '',
      'min' => 0,
      'prefix' => '',
      'suffix' => '',
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 0,
      'module' => 'number',
      'settings' => array(),
      'type' => 'number',
      'weight' => 5,
    ),
  );

  // Exported field_instance: 'farm_plan-grazing-field_expected_rotations'.
  $field_instances['farm_plan-grazing-field_expected_rotations'] = array(
    'bundle' => 'grazing',
    'default_value' => NULL,
    'deleted' => 0,
    'description' => '',
    'display' => array(
      'default' => array(
        'label' => 'inline',
        'module' => 'number',
        'settings' => array(
          'decimal_separator' => '.',
          'prefix_suffix' => TRUE,
          'scale' => 0,
          'thousand_separator' => '',
        ),
        'type' => 'number_integer',
        'weight' => 4,
      ),
    ),
    'entity_type' => 'farm_plan',
    'field_name' => 'field_expected_rotations',
    'label' => 'Expected number of rotations',
    'required' => 0,
    'settings' => array(
      'max' => '',
      'min' => 0,
      'prefix' => '',
      'suffix' => '',
      'user_register_form' => FALSE,
    ),
    'widget' => array(
      'active' => 0,
      'module' => 'number',
      'settings' => array(),
      'type' => 'number',
      'weight' => 6,
    ),
  );

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
          'show_remaining_days' => 0,
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
        'no_fieldset' => 1,
        'text_parts' => array(),
        'year_range' => '-10:+3',
      ),
      'type' => 'date_popup',
      'weight' => 2,
    ),
  );

  // Exported field_instance: 'farm_plan-grazing-field_farm_grazing_factors'.
  $field_instances['farm_plan-grazing-field_farm_grazing_factors'] = array(
    'bundle' => 'grazing',
    'default_value' => NULL,
    'deleted' => 0,
    'description' => 'Gather the team of people that implemented last years plan and that will be responsible for this year\'s plan. Consider your goals and the goals for the landscape you wish to create. Make a list of factors that will touch on livestock, wildlife, crops, haying, etc. Think through how the entire ranch will be used considering the number of cells, herds, stocking rates, cropping, drought reserve and biological monitoring. Enter all ideas in this planning factors field for future reference.',
    'display' => array(
      'default' => array(
        'label' => 'above',
        'settings' => array(),
        'type' => 'hidden',
        'weight' => 5,
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
      'weight' => 4,
    ),
  );

  // Exported field_instance: 'farm_plan-grazing-field_grazing_growing_season'.
  $field_instances['farm_plan-grazing-field_grazing_growing_season'] = array(
    'bundle' => 'grazing',
    'default_value' => array(
      0 => array(
        'value' => 1,
      ),
    ),
    'deleted' => 0,
    'description' => 'Does this plan take place during the growing season? Uncheck this box if your paddocks will not have actively growing forage during the date range of the plan. This will affect some calculations and recommendations when you are planning your rotations, and will utilize the settings below.',
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
      'weight' => 3,
    ),
  );

  // Translatables
  // Included for use with string extractors like potx.
  t('Date range');
  t('Days bulk feeding');
  t('Days of drought reserve');
  t('Define the date range for this grazing plan.');
  t('Does this plan take place during the growing season? Uncheck this box if your paddocks will not have actively growing forage during the date range of the plan. This will affect some calculations and recommendations when you are planning your rotations, and will utilize the settings below.');
  t('Expected number of rotations');
  t('Gather the team of people that implemented last years plan and that will be responsible for this year\'s plan. Consider your goals and the goals for the landscape you wish to create. Make a list of factors that will touch on livestock, wildlife, crops, haying, etc. Think through how the entire ranch will be used considering the number of cells, herds, stocking rates, cropping, drought reserve and biological monitoring. Enter all ideas in this planning factors field for future reference.');
  t('Growing season');
  t('Planning factors');

  return $field_instances;
}
