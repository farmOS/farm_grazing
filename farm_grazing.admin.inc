<?php

/**
 * Grazing types config form.
 */
function farm_grazing_config_animal_types_form($form, &$form_state) {

  // Load the animal types.
  $animal_types = farm_grazing_animal_types();

  // Help text.
  $form['help'] = array(
    '#type' => 'markup',
    '#markup' => t('This form allows you to edit the animal type options used in the grazing planning process.'),
    '#prefix' => '<p>',
    '#suffix' => '</p>',
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
      '#type' => 'textfield',
      '#default_value' => $type['name'],
      '#required' => TRUE,
    );

    // DMI factor.
    $form['types'][$id]['dmi_factor'] = array(
      '#type' => 'textfield',
      '#default_value' => $type['dmi_factor'],
      '#required' => TRUE,
    );

    // Remove checkbox.
    $form['types'][$id]['remove'] = array(
      '#type' => 'checkbox',
    );
  }

  // Submit button.
  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );

  return $form;
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
