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
function farm_grazing_plan_projections_form($form, &$form_state, $plan_obj, $herd_id) {

    // Load utility functions.
    module_load_include('inc', 'farm_grazing', 'farm_grazing.utils');

    // Add herds CSS.
    drupal_add_css(drupal_get_path('module', 'farm_grazing') . '/css/herds.css');

    // get the plan id from the plan object
    $plan = $plan_obj->id;

    $wrapper = entity_metadata_wrapper('farm_plan', $plan);
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

    $form['plan_id'] = array(
      '#type' => 'value',
      '#value' => $plan_id,
    );

    // Return markup.
    $form['text'] = array(
      '#markup' => t('<p>' .
        t('You can enter births, culls and avg. weight changes for the herd(s) on a month by month basis over the plan. The initial headcount is based on the arrival and departure dates definded for the animals in the herd and total monthly head count is based on that plus births and minus culls.') .
        '</p>' .
        ''),
    );


    // fetch the data we nesd to build the form
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
        asort($data);

        $titles = array(
            'births' => t('Births'),
            'culls' => t('Culls'),
            'weight' => t('Avg Weight'),
            'head_count' => t('Plan Head Count'),
            'total_hc' => t('Projected Head Count'),
        );

        $form['section'][$herd_id]['table'] = array();
        foreach ($data as $month => $items) {
            $form['section'][$herd_id]['table'][$month] = array();
            foreach ($items as $key => $val) {
                if ($key == 'proj_id') {
                    $form['section'][$herd_id]['table'][$month][$key] = array(
                        '#type' => 'hidden',
                        '#value' => $val,
                    );
                }
                elseif ($key == 'head_count' or $key == 'total_hc') {
                    $form['section'][$herd_id]['table'][$month][$key] = array(
                        '#prefix' => '<p>',
                        '#markup' => $val,
                        '#suffix' => '</p>',
                        '#title' => $titles[$key],
                    );
                }
                else {
                    $form['section'][$herd_id]['table'][$month][$key] = array(
                        '#type' => 'textfield',
                        '#title' => $titles[$key],
                        'default_value' => $val,
                    );
                }
            }
        }
    }

    return $form;
}


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
            $data = farm_grazing_get_herd_projections($plan_id, $herd_id, $plan_months);
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
    $p_start_ym = new Date();
    $p_start_ym->setTimestamp($p_start_ts);
    $p_start_ym = $p_start_ym->format('Ym');
    $p_end_ts = $wrapper->field_farm_date_range->value2->value();
    $p_end_ym = new Date();
    $p_end_ym->setTimestamp($p_end_ts);
    $p_end_ym = $p_end_ym->format('Ym');

    $herd_data = array();
    farm_grazing_get_grazing_group_detailed_info($herd_id, $herd_data);

    // cycle through the animals in the head and the months in the plan
    // and summarize iheadcnt and avg_weight by month

    $data = array();
    foreach ($herd_data as $rec) {
        $start_date = date_create_from_format('Y-m-d', $rec['arrival']);
        $end_date = date_create_from_format('Y-m-d', $rec['departure']);

        // If the start/end date are empty or outside the plan dates
        // use the plan's start/end date.
        if (empty($start_date) or $start_date < $p_start_ts) {
            $start_date = new DateTime();
            $start_date->setTimestamp($p_start_ts);
        }
        if (empty($end_date) or $end_date > $p_end_ts) {
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
        for ($i = 0; $i <= $plan_months[2]; $i++) {
            $key = $date->format('Ym');
            $data[$key]['proj_id'] = 0;
            $data[$key]['head_count'] += $rec['head_count'];
            $data[$key]['weight'] += $rec['head_count'] * $rec['weight'];
            $data[$key]['births'] = 0;
            $data[$key]['culls'] = 0;

            // TODO: do we need to add an error class if there is missing animal data??
            // like: farm_grazing.herds.manage.inc:273

            // increment the month
            $date->modify('+1 month');
        }

        // convert sum weight to average weight
        foreach ($data as $key => $val) {
            if ($val['head_count'] > 0) {
                $data[$key]['weight'] = $data[$key]['weight'] / $val['head_count'];
            }
        }
    }

    // get database records from farm_grazing_herd_projection for plan_id, herd_id order by year_month

    $result = db_query('SELECT * FROM {farm_grazing_herd_projections} WHERE plan_id=:plan_id and herd_id=:herd_id ORDER BY year_month', array(':plan_id' => $plan_id, ':herd_id' => $herd_id));
    foreach ($result as $rec) {
        $key = '' . $rec->year_month;
        if (array_key_exists($key, $data)) {
            // add fields from DB and over write the avg_weight with the DB value
            $data[$key]['proj_id'] = $rec->proj_id;
            $data[$key]['births'] = $rec->births;
            $data[$key]['culls'] = $rec->culls;
            $data[$key]['weight'] = $rec->avg_weight;
        }
        elseif (!array_key_exists($key, $data) or $key < $p_start_ym or $key > $p_end_ym) {
            // if the key does not exist then we have no initial head count so something is wrong
            // if the key is outside the paln dates then we don't need this record, probably the plan dates changed
            db_delete('farm_grazing_herd_projections')
                ->condition('proj_id', $rec->proj_id)
                -execute();
        }

        $data[$key]['total_hc'] = $data[$key]['head_count'] + $data[$key]['births'] - $data[$key]['culls'];
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

}

/**
 * Theme handler for farm_grazing_get_herd_projections_form
 *
 */
function theme_farm_grazing_get_herd_projections_form(&$vars) {
    $form = $vars['form'];
    $output = '';

    $output .= drupal_render($form['plan_id']);
    $output .= drupal_render($form['text']);

    foreach ($form['section'] as $herd_id => $herd) {

        $output .= drupal_render($form['section'][$herd_id]['title']);
        
        $header = array('');

        $rows = array();

        $output .= theme('table', array(
            'header' => $header,
            'rows' => $rows,
            'empty' => t('No data available'),
            'attributes' => array('id' => 'herd_projection_table_' . $herd_id),
        ));
    }

    return $output;
}

