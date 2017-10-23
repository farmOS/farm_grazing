<?php

include_once "farm_grazing.utils.inc";

/**
 * Set paddock productivity form callback
 */
function farm_grazing_plan_productivity_form($form, &$form_state, $plan_obj) {
  
  // Set the page title
  drupal_set_title(t('Rate Paddock Productivity'));

  // get the plan id from the plan object
  $plan = $plan_obj->id;

  $form['#tree'] = true;

  // Return markup
  $form['text'] = array(
    '#markup' => t('<p>Enter a value for each paddock in the Forage Quality field, then click the [Save] button. See the Help button for more information.</p>'),
  );

  $form['plan'] = array(
    '#type' => 'value',
    '#value' => $plan,
  );

  $header = array(
    t('Name'),
    t('Forage Quality'),
    t('Est. Relative Quality'),
  );

  // get a list of selected paddocks
  $selected = get_selected_paddock_quality_for_plan($plan);

  $form['paddocks'] = array(
    '#theme' => 'farm_grazing_paddocks_productivity',
    '#header' => $header,
  );

  $num = 0;
  $pad_avg = 0;
  $tot_avg = 0;

  foreach ($selected as $paddock_id => $quality) {
    $term = taxonomy_term_load($paddock_id);
    $form['paddocks'][$paddock_id]['name'] = array(
      '#markup' => $term->name,
    );
    $form['paddocks'][$paddock_id]['quality'] = array(
      '#type' => 'textfield',
      '#default_value' => $quality,
      '#size' => 30,
      '#maxlength' => 30,
    );
    $pad_avg = $pad_avg + $quality;

    $tot_quality = $quality * farm_area_calculate_area($paddock_id);
    $form['paddocks'][$paddock_id]['total'] = array(
      '#prefix' => '<p style="text-align:right">',
      '#markup' => round($tot_quality, 1),
      '#suffix' => '</p>',
    );
    $tot_avg = $tot_avg + $tot_quality;
    $num++;
  }

  $pad_avg = $pad_avg / $num;
  $tot_avg = $tot_avg / $num;

  $form['summary']['avg_rating'] = array(
    '#prefix' => '<p>' . t('Average AD/A or Rating: '),
    '#markup' => round($pad_avg, 1),
    '#suffix' => '</p>',
  );

  $form['summary']['avg_tot_rating'] = array(
    '#prefix' => '<p>' . t('Average Total AD/A or Rating: '),
    '#markup' => round($tot_avg, 1),
    '#suffix' => '</p>',
  );

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#submit' => array('farm_grazing_plan_productivity_form_submit'),
    '#validate' => array('farm_grazing_plan_productivity_form_validate'),
  );

  return $form;
}

/**
 * Validate handler for farm_grazing_paddocks_productivity_form
 */
function farm_grazing_plan_productivity_form_validate($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['input'], true));

  foreach ($form_state['input']['paddocks'] as $id => $val) {
    if (! is_numeric($val['quality']) or $val['quality'] < 0.0) {
      form_set_error('quality', t('Forage Quality must be numeric >= 0.0'));
    }
  }
}


/**
 * Process the paddock productivity form on submit
 */
function farm_grazing_plan_productivity_form_submit($form, &$form_state) {

  // debug output
  //$fp = fopen('php://stderr', 'a');
  //fputs($fp, print_r($form_state['values'], true));

  $plan = $form_state['values']['plan'];

  foreach ($form_state['values']['paddocks'] as $key => $val) {
    db_update('farm_grazing_plan_paddock')
      ->fields(array(
        'quality' => $val['quality'],
      ))
      ->condition('plan_id', $plan)
      ->condition('paddock_id', $key)
      ->execute();
  }
}

