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
  $field_group->parent_name = 'group_farm_tabs';
  $field_group->data = array(
    'label' => 'General settings',
    'weight' => '1',
    'children' => array(
      0 => 'field_farm_date_range',
      1 => 'name',
    ),
    'format_type' => 'field_group_easy_responsive_tabs_nav_item',
    'format_settings' => array(
      'label' => 'General settings',
      'instance_settings' => array(
        'required_fields' => 1,
        'classes' => '',
      ),
    ),
  );
  $field_groups['group_farm_general|farm_plan|grazing|form'] = $field_group;

  $field_group = new stdClass();
  $field_group->disabled = FALSE; /* Edit this to true to make a default field_group disabled initially */
  $field_group->api_version = 1;
  $field_group->identifier = 'group_farm_misc|farm_plan|grazing|form';
  $field_group->group_name = 'group_farm_misc';
  $field_group->entity_type = 'farm_plan';
  $field_group->bundle = 'grazing';
  $field_group->mode = 'form';
  $field_group->parent_name = 'group_farm_tabs';
  $field_group->data = array(
    'label' => 'Miscellaneous',
    'weight' => '3',
    'children' => array(
      0 => 'field_farm_grazing_factors',
    ),
    'format_type' => 'field_group_easy_responsive_tabs_nav_item',
    'format_settings' => array(
      'formatter' => '',
      'instance_settings' => array(
        'classes' => 'group-farm-misc field-group-field_group_easy_responsive_tabs_nav_item',
        'required_fields' => 1,
      ),
    ),
  );
  $field_groups['group_farm_misc|farm_plan|grazing|form'] = $field_group;

  $field_group = new stdClass();
  $field_group->disabled = FALSE; /* Edit this to true to make a default field_group disabled initially */
  $field_group->api_version = 1;
  $field_group->identifier = 'group_farm_tabs|farm_plan|grazing|form';
  $field_group->group_name = 'group_farm_tabs';
  $field_group->entity_type = 'farm_plan';
  $field_group->bundle = 'grazing';
  $field_group->mode = 'form';
  $field_group->parent_name = '';
  $field_group->data = array(
    'label' => 'Tabs',
    'weight' => '0',
    'children' => array(
      0 => 'group_farm_general',
      1 => 'group_non_growing',
      2 => 'group_farm_misc',
    ),
    'format_type' => 'field_group_easy_responsive_tabs_nav',
    'format_settings' => array(
      'label' => 'Tabs',
      'instance_settings' => array(
        'type' => 'vertical',
        'width' => 'auto',
        'fit' => '1',
        'closed' => '0',
        'activetab_bg' => '',
        'inactive_bg' => '',
        'active_border_color' => '',
        'active_content_border_color' => '',
        'tabidentify' => '',
        'required_fields' => 1,
        'classes' => 'group-farm-tabs field-group-field_group_easy_responsive_tabs_nav',
      ),
    ),
  );
  $field_groups['group_farm_tabs|farm_plan|grazing|form'] = $field_group;

  $field_group = new stdClass();
  $field_group->disabled = FALSE; /* Edit this to true to make a default field_group disabled initially */
  $field_group->api_version = 1;
  $field_group->identifier = 'group_non_growing|farm_plan|grazing|form';
  $field_group->group_name = 'group_non_growing';
  $field_group->entity_type = 'farm_plan';
  $field_group->bundle = 'grazing';
  $field_group->mode = 'form';
  $field_group->parent_name = 'group_farm_tabs';
  $field_group->data = array(
    'label' => 'Non-growing season',
    'weight' => '2',
    'children' => array(
      0 => 'field_grazing_growing_season',
      1 => 'field_days_bulk_feeding',
      2 => 'field_days_of_drought_reserve',
      3 => 'field_expected_rotations',
    ),
    'format_type' => 'field_group_easy_responsive_tabs_nav_item',
    'format_settings' => array(
      'label' => 'Non-growing season',
      'instance_settings' => array(
        'required_fields' => 1,
        'classes' => '',
      ),
    ),
  );
  $field_groups['group_non_growing|farm_plan|grazing|form'] = $field_group;

  // Translatables
  // Included for use with string extractors like potx.
  t('General settings');
  t('Miscellaneous');
  t('Non-growing season');
  t('Tabs');

  return $field_groups;
}
