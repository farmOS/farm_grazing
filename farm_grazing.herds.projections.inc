<?php

/**
 * @file
 * Grazing plan herd projections form.
 */

/**
 * get list of herds in plan
 * foreach herd
 *   fetch data from farm_grazing_herds_projections DB table
 *   generate a table like this for plan months
 *
 *     ## Herd: Herd 1
 *
 *     xxxx             | Jan | Feb | Mar | Apr | May | ...
 *     ---------------- | --- | --- | --- | --- | --- | ---
 *     Avg. Weight      | ... | ... | ... | ... | ... | ...
 *     Initial Head Cnt | ... | ... | ... | ... | ... | ...
 *     Births           | ... | ... | ... | ... | ... | ...
 *     Culls            | ... | ... | ... | ... | ... | ...
 *     Head Count       | ... | ... | ... | ... | ... | ...
 *
 *   repeat for additional herds
 *
 *   ## [Save]
 *
 * add form validation and submit handlers
 *
 * Where:
 *   - Initial Head Cnt comes from arrival and departure dates
 *   - Head Count is computed as Initial + births - Culls
 *   - and these two fields are not editable
 */

/**
 * Plan Herd projections page callback.
 */
function farm_grazing_plan_projections_form($form, &$form_state, $plan_obj) {

    // Load utility functions.
    module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

    // Add herds CSS.
    drupal_add_css(drupal_get_path('module', 'farm_grazing') . '/css/herds.css');

    // get the plan id from the plan object
    $plan_id = $plan_obj->id;

    $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
    $start_ts = $wrapper->field_farm_date_range->value->value();
    $end_ts = $wrapper->field_farm_date_range->value2->value();

    $start_date = new DateTime();
    $start_date->setTimestamp($start_ts);

    $end_date = new DateTime();
    $end_date->setTimestamp($end_ts);

    // get the DateInterval between them
    $diff = $start_date->diff($end_date);

    // generate array of months between start and end
    $num_months = $diff->y * 12 + $diff->m;
    $date = $start_date;

    $header = array('&nbsp;');

    // generate the header text
    $helper = array();
    for ($i = 0; $i <= $num_months; $i++) {
        $key = $date->format('Ym');
        $helper[$key] = $i;
        $header[] = $date->format('M Y');
        // increment the month
        $date->modify('+1 month');
    }

    // Set the page title.
    drupal_set_title(t('Herd Projections for Plan'));

    $form['#tree'] = TRUE;
    $form['#theme'] = 'farm_grazing_plan_projections_form';

    $form['plan_id'] = array(
      '#type' => 'value',
      '#value' => $plan_id,
    );

    // Return markup.
    $form['text'] = array(
      '#markup' => t('<p>' .
        t('You can enter births, culls and avg. weight changes for the herd(s) on a month by month basis over the plan. The initial headcount is based on the arrival and departure dates defined for the animals in the herd and total monthly head count is based on that plus births and minus culls.') .
        '</p>' .
        ''),
    );


    // fetch the data we need to build the form
    $herds = farm_grazing_get_herd_projections_for_plan($plan_id);

    // build the form from the data structure

    foreach ($herds as $herd) {
        $herd_id = $herd['herd_id'];

        $form['section'][$herd_id] = array();
        $form['section'][$herd_id]['title'] = array(
            '#prefix' => '<h4>',
            '#markup' => t('Herd: ') . $herd['name'],
            '#suffix' => '</h4>',
        );

        // sort the data by the months
        $data = $herd['data'];
        //asort($data);

        $titles = farm_grazing_herd_projections_titles();

        $form['section'][$herd_id]['table'] = array();
        foreach ($data as $key => $months) {
            foreach (array_keys($months) as $month) {
                $id = "proj-$plan_id-$herd_id-$month-0";
                $val = $data[$key][$month];
                if ($key == 'proj_id') {
                    $id = "proj-$plan_id-$herd_id-$month-$val";
                }
                elseif ($key == 'head_count' or $key == 'total_hc') {
                    $form['section'][$herd_id]['table'][$key][$month] = array(
                        '#prefix' => '<p>',
                        '#markup' => $val,
                        '#suffix' => '</p>',
                    );
                }
                elseif ($key == 'weight') {
                    $form['section'][$herd_id]['table'][$key][$month] = array(
                        '#type' => 'textfield',
                        '#default_value' => $val,
                        '#title' => $titles[$key],
                        '#title_display' => 'invisible',
                        '#attributes' => array('id' => "$id-$key"),
                        '#element_validate' => array('element_validate_number_positive'),
                    );
                }
                else {
                    $form['section'][$herd_id]['table'][$key][$month] = array(
                        '#type' => 'textfield',
                        '#default_value' => $val,
                        '#title' => $titles[$key],
                        '#title_display' => 'invisible',
                        '#attributes' => array('id' => "$id-$key"),
                        '#element_validate' => array('element_validate_integer', 'element_validate_number_positive'),
                    );
                }
            }
        }
    }

    $form['submit'] = array(
        '#type' => 'submit',
        '#value' => t('Save Changes'),
        '#validate' => array('farm_grazing_get_herd_projections_form_validate'),
        '#submit' => array('farm_grazing_get_herd_projections_form_submit'),
    );

    return $form;
}


/**
 * element_validate_number_positive($element, &$form_state)
 */
function element_validate_number_positive($element, &$form_state) {
    $value = $element['#value'];
    if ($value != '' && (! is_numeric($value) || $value < 0)) {
        form_error($element, t('%name must be a number >= 0.',
            array('%name' => $element['#title'])));
    }
}


/**
 * farm_grazing_herd_projections_titles()
 *
 * @return array of row titles
 */
function farm_grazing_herd_projections_titles() {
    return array(
        'proj_id' => '',
        'weight' => t('Avg Weight'),
        'head_count' => t('Plan Head Count'),
        'births' => t('Births'),
        'culls' => t('Culls'),
        'total_hc' => t('Projected Head Count'),
    );
};


/**
 * farm_grazing_get_herd_projections_for_plan($plan_id)
 *
 * Collect the data need to populate the farm_grazing_plan_projections_form
 *
 * @param $plan_id
 *   The plan id
 *
 * @return array( array(
 *      herd_id => int
 *      name => string
 *      data => array(
 *          year_month => array(
 *              proj_id => int | 0      db table rec or 0 if no record yet
 *              weight => float         count by month in plan
 *              head_count => int       initial head cnt by month
 *              births => int           num births by month
 *              culls => init           num culls by month
 *          ),
 *      ),
 *    ),
 *  );
 */

function farm_grazing_get_herd_projections_for_plan($plan_id) {

   $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
   $start_ts = $wrapper->field_farm_date_range->value->value();
   $end_ts = $wrapper->field_farm_date_range->value2->value();

   $plan_months = farm_grazing_get_num_months_between($start_ts, $end_ts);

   // Get a list of all herds in the plan.
   $plan_herds = array();
   $result = db_query('SELECT * FROM {farm_grazing_herds} WHERE plan_id=:plan_id', array(':plan_id' => $plan_id));
   foreach ($result as $row) {
        if (!empty($row->herd_id)) {
            $herd = farm_asset_load($row->herd_id);
            $data = farm_grazing_get_herd_projections($plan_id, $row->herd_id, $plan_months);
            $plan_herds[] = array(
                'herd_id' => $row->herd_id,
                'name' => $herd->name,
                'data' => $data,
            );
        }
   }

   return $plan_herds;
}

/**
 * farm_grazing_get_herd_projections($plan_id, $herd_id, $plan_months)
 *
 * return projections for a herd in a plan
 * if no record is found in the farm_grazing_herd_projections for the herd
 * return reasonable default values that can be used to populate the form
 *
 * @param $plan_id
 *  The plan ID
 *
 *  @param $herd_id
 *      The herd ID
 *
 *  @param $plan_months
 *      Array(year_start, month_start, num_months)
 *
 *  @return array(          data for herd_id
 *    year_month => array(          YYYYMM for the month in the plan
 *      proj_id => int | 0          db table rec or 0 if no record yet
 *      weight => float             avg weight for herd in this month
 *      head_count => int           initial head cnt this month
 *      births => int               num births this month
 *      culls => int                num culls this month
 *    ),
 *    ...                    repeated for num_months
 *  )
 *
 *  Issues that need to be dealt with here:
 *  * if plan dates changes, this need to be self healing
 *    - if plan date changes extends the month range, new records need to be defined
 *    - if plan date changes shrinks the month range, excess records should be removed
 *
 */

function farm_grazing_get_herd_projections($plan_id, $herd_id, $plan_months) {

    // get initial head count based on animal arrival and departure dates

    // get plan start and end dates
    $wrapper = entity_metadata_wrapper('farm_plan', $plan_id);
    $p_start_ts = $wrapper->field_farm_date_range->value->value();
    $p_start_ym = new DateTime();
    $p_start_ym->setTimestamp($p_start_ts);
    $p_start_ym = $p_start_ym->format('Ym');
    $p_end_ts = $wrapper->field_farm_date_range->value2->value();
    $p_end_ym = new DateTime();
    $p_end_ym->setTimestamp($p_end_ts);
    $p_end_ym = $p_end_ym->format('Ym');

    $herd_data = array();
    farm_grazing_get_grazing_group_detailed_info($herd_id, $herd_data);

    // initialize data array for all months in plan

    $date = new DateTime();
    $date->setDate($plan_months[0], $plan_months[1], 1);
    $data = array();
    for ($i=0; $i<$plan_months[2]; $i++) {
        $key = $date->format('Ym');
        $data['proj_id'][$key] = 0;
        $data['head_count'][$key] = 0;
        $data['births'][$key] = 0;
        $data['culls'][$key] = 0;
        $data['total_hc'][$key] = 0;
        $data['weight'][$key] = 0;

        $date->modify('+1 month');
    }

    // cycle through the animals in the head and the months in the plan
    // and summarize headcnt and avg_weight by month

    foreach ($herd_data as $rec) {

        if (empty($rec['arrival']) or $rec['arrival'] == '') {
            $start_date = new DateTime();
            $start_date->setTimestamp($p_start_ts);
        }
        else {
            $start_date = date_create_from_format('Y-m-d', $rec['arrival']);
        }
        $start_date_ts = $start_date->getTimestamp();
        if (empty($rec['departure']) or $rec['departure'] == '') {
            $end_date = new DateTime();
            $end_date->setTimestamp($p_end_ts);
        }
        else {
            $end_date = date_create_from_format('Y-m-d', $rec['departure']);
        }
        $end_date_ts = $end_date->getTimestamp();

        // If the start/end date are empty or outside the plan dates
        // use the plan's start/end date.
        if ($start_date_ts < $p_start_ts) {
            $start_date = new DateTime();
            $start_date->setTimestamp($p_start_ts);
        }
        if ($end_date_ts > $p_end_ts) {
            $end_date = new DateTime();
            $end_date->setTimestamp($p_end_ts);
        }

        // get the DateInterval between them
        $diff = $start_date->diff($end_date);

        // generate array of months between start and end
        $num_months = $diff->y * 12 + $diff->m;

        // force the date to first day of month
        list($y, $m, $d) = explode(',', $start_date->format('Y,m,d'));
        $date = new DateTime();
        $date->setDate($y, $m, 1);

        // sum by month
        for ($i = 0; $i < $num_months; $i++) {
            $key = $date->format('Ym');
            $data['proj_id'][$key] = 0;
            $data['head_count'][$key] += $rec['head_count'];
            $data['weight'][$key] += $rec['head_count'] * $rec['weight'];
            $data['births'][$key] = 0;
            $data['culls'][$key] = 0;

            // TODO: do we need to add an error class if there is missing animal data??
            // like: farm_grazing.herds.manage.inc:273

            // increment the month
            $date->modify('+1 month');
        }

        // convert sum weight to average weight (avoid divide by zero)
        foreach (array_keys($data['head_count']) as $month) {
            $val = $data['head_count'][$month];
            if ($val > 0) {
                $data['weight'][$month] /= $val;
            }
        }
    }

    // get database records from farm_grazing_herd_projection for plan_id, herd_id order by year_month

    $result = db_query("SELECT * FROM {farm_grazing_herd_projections} WHERE plan_id=:plan_id and herd_id=:herd_id ORDER BY 'year_month'", array(':plan_id' => $plan_id, ':herd_id' => $herd_id));
    foreach ($result as $rec) {
        $month = '' . $rec->year_month;
        if (array_key_exists($month, $data['head_count'])) {
            // add fields from DB and over write the avg_weight with the DB value
            $data['proj_id'][$month] = $rec->proj_id;
            $data['births'][$month] = $rec->births;
            $data['culls'][$month] = $rec->culls;
            $data['weight'][$month] = $rec->avg_weight;
        }
        elseif (!array_key_exists($month, $data['head_count']) or $month < $p_start_ym or $key > $p_end_ym) {
            // if the key does not exist then we have no initial head count so something is wrong
            // if the key is outside the paln dates then we don't need this record, probably the plan dates changed
            db_delete('farm_grazing_herd_projections')
                ->condition('proj_id', $rec->proj_id)
                -execute();
        }

    }

    // compute total head count after births and culls

    foreach (array_keys($data['head_count']) as $month) {
        $data['total_hc'][$month] = $data['head_count'][$month] + $data['births'][$month] - $data['culls'][$month];
    }

    return $data;
}


/**
 * Validation handler for farm_grazing_get_herd_projections_form
 *
 */
function farm_grazing_get_herd_projections_form_validate($form, &$form_state) {
}


/**
 * Submit handler for farm_grazing_get_herd_projections_form
 *
 */
function farm_grazing_get_herd_projections_form_submit($form, &$form_state) {
    dpm('submit');
    dpm($form);
}

/**
 * Theme handler for farm_grazing_get_herd_projections_form
 *
 */
function theme_farm_grazing_plan_projections_form(&$vars) {
    $form = $vars['form'];
    $output = '';

    $output .= drupal_render($form['plan_id']);
    $output .= drupal_render($form['text']);

    $titles = farm_grazing_herd_projections_titles();

    $header = array('&nbsp;');
    $rows = array();
    $first = TRUE;
    foreach (element_children($form['section']) as $herd_id) {
        $output .= drupal_render($form['section'][$herd_id]['title']);
        foreach (element_children($form['section'][$herd_id]['table']) as $key) {
            if ($first) {
                foreach (element_children($form['section'][$herd_id]['table'][$key]) as $month) {
                    $months[] = $month;
                    $header[] = $month;
                }
                $first = FALSE;
            }

            $row_title = array(
                '#prefix' => '<p>',
                '#markup' => $titles[$key],
                '#suffix' => '</p>',
            );

            $data = array(drupal_render($row_title));

            foreach ($months as $month) {
                $item =& $form['section'][$herd_id]['table'][$key][$month];
                $data[] = drupal_render($item);
            }

            $rows[] = array('data' => $data);
        }
    }

    $table = array(
        'header' => $header,
        'rows' => $rows,
        'empty' => t('No data available'),
        'attributes' => array('id' => 'herd_projection_table_' . $herd_id),
    );

    $output .= theme('table', $table);

    $output .= drupal_render_children($form);

    return $output;
}

