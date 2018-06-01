<?php

/**
 * @file
 * Grazing herd detail form.
 */

/**
 * Herd detail page callback.
 */
function farm_grazing_plan_herds_detail_form($form, &$form_state, $plan_obj, $herd_id) {

  // Load utility functions.
  module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

  // Add herds CSS.
  drupal_add_css(drupal_get_path('module', 'farm_grazing') . '/css/herds.css');

  // get the plan id from the plan object
  $plan = $plan_obj->id;

  // Load the herd and generate its label.
  $herd_group = farm_asset_load($herd_id);
  $herd_name = entity_label('farm_asset', $herd_group);

  // Set the page title.
  drupal_set_title(t('Herd details: @herd_name', array('@herd_name' => $herd_name)));

  $form['#tree'] = TRUE;

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  // Load and assemble information about this herd.
  $herd_data = array();
  $herd_summary = farm_grazing_get_herd_data_and_summary($herd_id, $plan, $herd_data);
  $this_herd = array(
    'herd_id' => $herd_id,
    'herd_name' => $herd_name,
    'herd_data' => $herd_data,
    'summary' => $herd_summary,
  );

  // --------- Add table of herd details -----------------

  $type_options = farm_grazing_animal_type_options();

  foreach ($this_herd['herd_data'] as $rec) {

    // Create an array for the animal fields, and a reference to it.
    $form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']] = array();
    $animal_fields = &$form['herd_details'][$this_herd['herd_id']]['animals'][$rec['id']];

    // Animal record name.
    $animal_fields['name'] = array(
      '#markup' => l($rec['name'], 'farm/asset/' . $rec['id'], array('query' => array('destination' => current_path()))),
    );

    // Animal type.
    $animal_fields['type'] = array(
      '#title' => t('Animal type'),
      '#title_display' => 'invisible',
      '#type' => 'select',
      '#options' => $type_options,
      '#default_value' => !empty($rec['type']) ? $rec['type'] : '',
    );

    // Head count.
    $animal_fields['head_count'] = array(
      '#title' => t('Head count'),
      '#title_display' => 'invisible',
      '#type' => 'textfield',
      '#default_value' => $rec['head_count'],
      '#size' => 30,
      '#maxlength' => 30,
      '#element_validate' => array('element_validate_number'),
    );

    // Weight.
    $animal_fields['weight'] = array(
      '#title' => t('Average weight'),
      '#title_display' => 'invisible',
      '#type' => 'textfield',
      '#default_value' => $rec['weight'],
      '#size' => 30,
      '#maxlength' => 30,
      '#element_validate' => array('element_validate_number'),
    );

    $animal_fields['total_weight'] = array(
      '#markup' => $rec['weight'] * $rec['head_count'],
    );

    // SAU.
    $animal_fields['sau'] = array(
      '#markup' => round($rec['weight'] * $rec['head_count'] / 1000.0, 2),
    );

    // DMI.
    $animal_fields['dmi'] = array(
      '#markup' => round($rec['head_count'] * $rec['weight'] * $rec['dmi_factor'], 2),
    );

    // Planned arrival.
    $animal_fields['arrival'] = array(
      '#title' => t('Planned arrival'),
      '#title_display' => 'invisible',
      '#type' => 'date_popup',
      '#date_format' => 'M j Y',
      '#date_type' => DATE_FORMAT_UNIX,
      '#date_year_range' => '-10:+3',
      '#date_label_position' => 'none',
      '#default_value' => $rec['arrival'],
    );

    // Planned departure.
    $animal_fields['departure'] = array(
      '#title' => t('Planned departure'),
      '#title_display' => 'invisible',
      '#type' => 'date_popup',
      '#date_format' => 'M j Y',
      '#date_type' => DATE_FORMAT_UNIX,
      '#date_year_range' => '-10:+3',
      '#date_label_position' => 'none',
      '#default_value' => $rec['departure'],
    );

    // If the animal type or weight is empty...
    if (empty($rec['type']) || empty($rec['weight'])) {

      // If animal type is empty, add an error class to the dropdown.
      if (empty($rec['type'])) {
        $animal_fields['type']['#attributes']['class'] = array('error');
      }

      // If weight is empty, add an error class to the weight field, the SAU,
      // and the DMI.
      if (empty($rec['weight'])) {
        $animal_fields['weight']['#attributes']['class'] = array('error');
        $animal_fields['sau']['#prefix'] = '<span class="error">';
        $animal_fields['sau']['#suffix'] = '</span>';
        $animal_fields['dmi']['#prefix'] = '<span class="error">';
        $animal_fields['dmi']['#suffix'] = '</span>';
      }
    }
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save Changes'),
  );

  // Return the form.
  return $form;
}

/**
 * Validation handler for saving the herd information.
 */
function farm_grazing_plan_herds_detail_form_validate($form, &$form_state) {

  // If there are no herd details, bail.
  if (empty($form_state['values']['herd_details'])) {
    return;
  }

  // Iterate through the herds.
  foreach ($form_state['values']['herd_details'] as $herd_id => $herd_details) {

    // If there are no animals in the herd, skip it.
    if (empty($herd_details['animals'])) {
      continue;
    }

    // Iterate through the animals.
    foreach ($herd_details['animals'] as $animal_id => $animal_details) {

      // Get the original $form array for this animal.
      $animal_form = $form['herd_details'][$herd_id]['animals'][$animal_id];

      // If certain fields are modified, make sure they are not left blank.
      $required_fields = array(
        'type' => t('Animal type'),
        'head_count' => t('Head count'),
        'weight' => t('Weight'),
        'arrival' => t('Planned arrival'),
        'departure' => t('Planned departure'),
      );
      foreach ($required_fields as $field => $label) {
        if ($animal_form[$field]['#default_value'] != $animal_details[$field]) {
          if (empty($animal_details[$field])) {
            form_set_error('herd_details][' . $herd_id . '][animals][' . $animal_id . '][' . $field, $label . ' ' . t('is required.'));
          }
        }
      }
    }
  }
}

/**
 * Submit handler for saving the herd information.
 */
function farm_grazing_plan_herds_detail_form_submit($form, &$form_state) {

  // If there are no herd details, bail.
  if (empty($form_state['values']['herd_details'])) {
    return;
  }

  // Remember if values have been updated.
  $updated = FALSE;

  // Iterate through the herds.
  foreach ($form_state['values']['herd_details'] as $herd_id => $herd_details) {

    // If there are no animals in the herd, skip it.
    if (empty($herd_details['animals'])) {
      continue;
    }

    // Iterate through the animals.
    foreach ($herd_details['animals'] as $animal_id => $animal_details) {

      // Get the original $form array for this animal.
      $animal_form = $form['herd_details'][$herd_id]['animals'][$animal_id];

      // If the animal's type has changed...
      if ($animal_form['type']['#default_value'] != $animal_details['type']) {

        // Set the asset property value.
        farm_asset_property_set($animal_id, 'farm_grazing_animal_type', $animal_details['type']);
        $updated = TRUE;
      }

      // If the animal's head count has changed...
      if ($animal_form['head_count']['#default_value'] != $animal_details['head_count']) {

        // Create an inventory log.
        $animal = farm_asset_load($animal_id);
        if (!empty($animal)) {
          farm_inventory_set($animal, $animal_details['head_count']);
          $updated = TRUE;
        }
      }

      // If the animal's weight has changed...
      if ($animal_form['weight']['#default_value'] != $animal_details['weight']) {

        // Create a weight log.
        $animal = farm_asset_load($animal_id);
        if (!empty($animal)) {
          farm_livestock_weight_set($animal, $animal_details['weight'], 'lbs');
          $updated = TRUE;
        }
      }

      // If the animal's planned arrival has changed...
      if ($animal_form['arrival']['#default_value'] != $animal_details['arrival']) {

        // Set the asset property value.
        $planned_arrival = strtotime($animal_details['arrival']);
        farm_asset_property_set($animal_id, 'farm_grazing_planned_arrival', $planned_arrival);
        $updated = TRUE;
      }

      // If the animal's planned departure has changed...
      if ($animal_form['departure']['#default_value'] != $animal_details['departure']) {

        // Set the asset property value.
        $planned_departure = strtotime($animal_details['departure']);
        farm_asset_property_set($animal_id, 'farm_grazing_planned_departure', $planned_departure);
        $updated = TRUE;
      }
    }
  }

  // If any values were updated, show a message.
  if ($updated) {
    drupal_set_message(t('Animals were updated.'));
  }
}

/**
 * theme_farm_grazing_plan_herds_detail_form
 */
function theme_farm_grazing_plan_herds_detail_form(&$vars) {
  $form = $vars['form'];
  $output = '';

  // render table for herd details
  if (!empty($form['herd_details'])) {
    foreach (element_children($form['herd_details']) as $herd_id) {
      // render table titles
      $output .= drupal_render($form['herd_details'][$herd_id]['title']);
      $table_id = 'herd_details_' . $herd_id;

      // assemble the table rows
      $rows = array();
      if (!empty($form['herd_details'][$herd_id]['animals'])) {
        foreach (element_children($form['herd_details'][$herd_id]['animals']) as $aid) {
          $item =& $form['herd_details'][$herd_id]['animals'][$aid];

          // Assemble the row.
          $rows[] = array(
            'data' => array(
              drupal_render($item['name']),
              drupal_render($item['type']),
              drupal_render($item['head_count']),
              drupal_render($item['weight']),
              drupal_render($item['total_weight']),
              drupal_render($item['sau']),
              drupal_render($item['dmi']),
              drupal_render($item['arrival']),
              drupal_render($item['departure']),
            ),
          );
        }
      }

      // define the table
      $header = array(
        t('Animal Record'),
        t('Type'),
        t('Head Count'),
        t('Weight (Average)'),
        t('Total Weight'),
        t('SAU'),
        t('Daily Intake'),
        t('Planned Arrival'),
        t('Planned Departure'),
      );

      $output .= theme('table', array(
        'header' => $header,
        'rows' => $rows,
        'empty' => t('No animals found in herd'),
        'attributes' => array('id' => $table_id),
      ));
    }
  }

  // render any remaining elements
  $output .= drupal_render_children($form);

  return $output;
}
