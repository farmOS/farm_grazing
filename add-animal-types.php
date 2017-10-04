/*
  SQL to add some sample animal types to {farm_grazing_animal_types} table
  use http://host/devel/php to run it
*/
  $data = array(
    array(
      'name' => 'Beef cattle, lactating',
      'dmi_factor' => 2.25,
    ),
    array(
      'name' => 'Beef cattle, growing and finishing, slaughter stock',
      'dmi_factor' => 2.3,
    ),
    array(
      'name' => 'Dairy steers',
      'dmi_factor' => 2.3,
    ),
    array(
      'name' => 'Dairy heifers',
      'dmi_factor' => 2.5,
    ),
    array(
      'name' => 'Dairy cows, dry (small or large breed)',
      'dmi_factor' => 1.8,
    ),
    array(
      'name' => 'Goats, weaned, slaughter or replacement stock',
      'dmi_factor' => 2.25,
    ),
    array(
      'name' => 'Goats, brood or lactating',
      'dmi_factor' => 4.0,
    ),
    array(
      'name' => 'Sheep, weaned, slaughter or replacement stock',
      'dmi_factor' => 3.3,
    ),
    array(
      'name' => 'Sheep, brood or lactating',
      'dmi_factor' => 3.65,
    ),
  );

  foreach ($data as $row) {
    db_insert('farm_grazing_animal_types')
      ->fields($row)
      -> execute();
  }

$query = db_query('select * from farm_grazing_animal_types');
$records = $query->fetchAll();
print_r($records);

//db_query('drop table farm_grazing_rotations cascade');
//db_query('drop table farm_grazing_paddock_exclusions cascade');
//db_query('drop table farm_grazing_plan_paddock cascade');
//db_query('drop table farm_grazing_plan_recovery cascade');
//db_query('drop table farm_grazing_herds cascade');
//db_query('drop table farm_grazing_animal_types cascade');

