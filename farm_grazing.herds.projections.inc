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

    // Set the page title.
    drupal_set_title(t('Manage Herds for Plan'));

    $form['#tree'] = TRUE;

    $form['plan'] = array(
      '#type' => 'value',
      '#value' => $plan,
    );

    // Return markup.
    $form['text'] = array(
      '#markup' => t('<p>' .
        t('You can enter births, culls and avg. weight changes for the herd(s) on a month by month basis over the plan. The initial headcount is based on the arrival and departure dates definded for the animals in the herd and total monthly head count is based on that plus births and minus culls.') .
        '</p>' .
        ''),
   );


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
 *          proj_id => int | 0           db table rec or 0 if no record yet
 *          month => int                 index of month from start of plan (0=month at start)
 *          weight => array(float, ...)  count by month in plan
 *          iheadcnt => array(int, ...)  initial head cnt by month
 *          births => array(int, ...)    num births by month
 *          culls => array(init, ...)    num culls by month
 *      )
 *    ), ...,
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
                'data' = $data,
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
 *  @return array(
 *    array(
 *      proj_id => int | 0           db table rec or 0 if no record yet
 *      year_month => int            integer YYYYMM for the month in the plan
 *      weight => float              avg weight for herd in this month
 *      iheadcnt => int              initial head cnt this month
 *      births => int                num births this month
 *      culls => int                 num culls this month
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

    foreach ($herd_data as $rec) {
        $row = array_fill(0, $plan_months[2], 0);
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
        for ($i = 0; $i <= $plan_months[2], $i++) {
            $key = $date->format('Ym');
            $row[$key]['head_count'] += $rec['head_count'];
            $row[$key]['weight'] += $rec['head_count'] * $rec['weight'];
            $row[$key]{'births'] = 0;
            $row[$key]['culls'] = 0;

            // TODO: do we need to add an error class if there is missing animal data??
            // like: farm_grazing.herds.manage.inc:273

            // increment the month
            $date->modify('+1 month');
        }

        // convert sum weight to average weight
        foreach ($row as $key => $val) {
            if ($val['head_count'] > 0) {
                $row[$key]['weight'] = $row[$key]['weight'] / $val['head_count'];
            }
        }
    }

    // get database records from farm_grazing_herd_projection for plan_id, herd_id order by year_month

    $result = db_query('SELECT * FROM {farm_grazing_herd_projections} WHERE plan_id=:plan_id and herd_id=:herd_id ORDER BY year_month', array(':plan_id' => $plan_id, ':herd_id' => $herd_id));
    foreach ($result as $rec) {
        $key = '' . $rec->year_month;
        if (array_key_exists($key, $row)) {
            // add fields from DB and over write the avg_weight with the DB value
            $row[$key]['proj_id'] = $rec->proj_id;
            $row[$key]['births'] = $rec->births;
            $row[$key]['culls'] = $rec->culls;
            $row[$key]['weight'] = $rec->avg_weight;
        }
        elseif (!array_key_exists($key, $row) or $key < $p_start_ym or $key > $p_end_ym) {
            // if the key does not exist then we have no initial head count so something is wrong
            // if the key is outside the paln dates then we don't need this record, probably the plan dates changed
            db_delete('farm_grazing_herd_projections')
                ->condition('proj_id', $rec->proj_id)
                -execute();
        }
    }

    // build data array to return checking for changes to plan dates

}
