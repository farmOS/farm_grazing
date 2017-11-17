<?php

/**
 * @file
 * Grazing configuration forms.
 */

/**
 * Grazing types config form.
 */
function farm_grazing_config_animal_types_form($form, &$form_state) {

  // Load the animal types.
  $animal_types = farm_grazing_animal_types();

  // Treat the form as a tree.
  $form['#tree'] = TRUE;

  // Help text.
  $form['help'] = array(
    '#type' => 'markup',
    '#markup' => t('This form allows you to edit the animal type options used in the grazing planning process. The DMI (dry matter intake) factor is used to compute the amount of dry matter intake an animal requires (DMI = animal weight x  DMI factor / 100).'),
    '#prefix' => '<p>',
    '#suffix' => '</p>',
  );

  // Save the array of animal types for comparison in validate/submit.
  $form['animal_types'] = array(
    '#type' => 'value',
    '#value' => $animal_types,
  );

  // Iterate through the animal types and add fields for each.
  foreach ($animal_types as $id => $type) {

    // Animal type ID (hidden value).
    $form['types'][$id]['type_id'] = array(
      '#type' => 'value',
      '#value' => $type['type_id'],
    );

    // Animal type name.
    $form['types'][$id]['name'] = array(
      '#title' => t('Animal type name'),
      '#title_display' => 'invisible',
      '#type' => 'textfield',
      '#default_value' => $type['name'],
      '#required' => TRUE,
      '#maxlength' => 255,
    );

    // DMI factor.
    $form['types'][$id]['dmi_factor'] = array(
      '#title' => t('DMI factor'),
      '#title_display' => 'invisible',
      '#type' => 'textfield',
      '#default_value' => $type['dmi_factor'],
      '#required' => TRUE,
    );

    // Remove checkbox.
    $form['types'][$id]['remove'] = array(
      '#type' => 'checkbox',
    );
  }

  // Fieldset for adding a new type.
  $form['add'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add animal type'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
  );
  $form['add']['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Animal type'),
    '#maxlength' => 255,
  );
  $form['add']['dmi_factor'] = array(
    '#type' => 'textfield',
    '#title' => t('DMI factor'),
    '#description' => t('What is the dry matter intake factor for this animal type?'),
    '#element_validate' => array('element_validate_number'),
  );

  // Submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
}

/**
 * Validate handler for the animal type configuration form.
 */
function farm_grazing_config_animal_types_form_validate($form, &$form_state) {

  // If we are adding a type, validate the fields.
  if (!empty($form_state['values']['add']['name']) || !empty($form_state['values']['add']['dmi_factor'])) {

    // Make sure both a name and DMI are set.
    if (empty($form_state['values']['add']['name'])) {
      form_set_error('add][name', t('Animal type name is required.'));
    }
    elseif (empty($form_state['values']['add']['dmi_factor'])) {
      form_set_error('add][dmi_factor', t('DMI factor is required.'));
    }

    // Make sure name is unique.
    if (!empty($form_state['values']['add']['name'])) {
      $name = $form_state['values']['add']['name'];
      $exists = db_query('SELECT COUNT(*) FROM {farm_grazing_animal_types} WHERE name = :name', array(':name' => $name))->fetchField();
      if (!empty($exists)) {
        form_set_error('add][name', t('An animal type named "%name" already exists.', array('%name' => $name)));
      }
    }
  }

  // Iterate through the animal types.
  foreach ($form_state['values']['types'] as $type_id => $type) {

    // If the animal type is being removed, check to see if it is in use.
    if (!empty($type['remove'])) {

      // Load usage information from the {farm_asset_property} table.
      $usage = db_query("SELECT COUNT(id) FROM {farm_asset_property} WHERE name = 'farm_grazing_animal_type' AND value = :type_id", array(':type_id' => $type_id))->fetchField();

      // Don't allow deleting types that are in use.
      if (!empty($usage)) {
        form_set_error('types][' . $type_id . '][delete', t('The animal type "%animal_type" cannot be deleted because it is in use on animals already.', array('%animal_type' => $form_state['values']['animal_types'][$type_id]['name'])));
      }
    }
  }
}

/**
 * Submit handler for the animal type configuration form.
 */
function farm_grazing_config_animal_types_form_submit($form, &$form_state) {

  // Get the saved animal types.
  $animal_types = $form_state['values']['animal_types'];

  // Iterate through the types.
  foreach ($form_state['values']['types'] as $type_id => $type) {

    // Compare the name and dmi_factor to the saved values to see if we should
    // update it.
    $update = FALSE;
    if (($type['name'] != $animal_types[$type_id]['name']) || ($type['dmi_factor'] != $animal_types[$type_id]['dmi_factor'])) {
      $update = TRUE;
    }

    // If the type is marked for removal, delete it.
    if (!empty($type['remove'])) {

      // Delete the record.
      db_query('DELETE FROM {farm_grazing_animal_types} WHERE type_id = :type_id', array(':type_id' => $type_id));

      // Do not update it, because it doesn't exist anymore!
      $update = FALSE;
    }

    // If we should update, then do so.
    if (!empty($update)) {
      drupal_write_record('farm_grazing_animal_types', $type, array('type_id'));
    }
  }

  // If a new type is being added, create it.
  if (!empty($form_state['values']['add']['name']) && !empty($form_state['values']['add']['dmi_factor'])) {
    $record = array(
      'name' => $form_state['values']['add']['name'],
      'dmi_factor' => $form_state['values']['add']['dmi_factor'],
    );
    drupal_write_record('farm_grazing_animal_types', $record);
  }

  // Display a generic message.
  drupal_set_message('Animal types were updated.');
}

/**
 * Theme function for the animal type configuration form.
 */
function theme_farm_grazing_config_animal_types_form(&$vars) {

  // Start an empty output string.
  $output = '';

  // Get the form array.
  $form = &$vars['form'];

  // Render the help text.
  $output .= drupal_render($form['help']);

  // Assemble the table rows.
  $rows = array();
  foreach (element_children($form['types']) as $type_id) {

    // Get the form item.
    $item = &$form['types'][$type_id];

    // Assemble the row.
    $rows[] = array(
      drupal_render($item['name']),
      drupal_render($item['dmi_factor']),
      drupal_render($item['remove']),
    );
  }

  // Define the table header.
  $header = array(
    t('Animal type'),
    t('DMI factor'),
    t('Remove'),
  );

  // Render the table into the the output.
  $output .= theme('table', array(
    'header' => $header,
    'rows' => $rows,
  ));

  // Render any remaining form elements.
  $output .= drupal_render_children($form);

  return $output;
}
