<?php
/**
 * @file
 * farm_grazing.field_group.inc
 */

/**
 * Implements hook_field_group_info().
 */
function farm_grazing_field_group_info() {
  $field_groups = array();

  $field_group = new stdClass();
  $field_group->disabled = FALSE; /* Edit this to true to make a default field_group disabled initially */
  $field_group->api_version = 1;
  $field_group->identifier = 'group_farm_general|farm_plan|grazing|form';
  $field_group->group_name = 'group_farm_general';
  $field_group->entity_type = 'farm_plan';
  $field_group->bundle = 'grazing';
  $field_group->mode = 'form';
  $field_group->parent_name = '';
  $field_group->data = array(
    'label' => 'General information',
    'weight' => '0',
    'children' => array(
      0 => 'field_grazing_growing_season',
      1 => 'field_farm_grazing_factors',
      2 => 'name',
    ),
    'format_type' => 'fieldset',
    'format_settings' => array(
      'formatter' => 'collapsible',
      'instance_settings' => array(
        'description' => '',
        'classes' => 'group-farm-general field-group-fieldset',
        'required_fields' => 1,
        'id' => '',
      ),
    ),
  );
  $field_groups['group_farm_general|farm_plan|grazing|form'] = $field_group;

  // Translatables
  // Included for use with string extractors like potx.
  t('General information');

  return $field_groups;
}
