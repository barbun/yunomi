<?php
/**
* @file
* Callbacks for services module resource hooks.
*/

/**
 * Returns the formatted results array for recipes.
 *
 * @param $arg
 *   The specified option of return result.
 * @return
 *   Formatted array or error if not found.
 *
 * @see node_load()
 */
function _yunomi_rest_recipe_retrieve($arg, $query, $moment = NULL, $menu = NULL, $preparation = NULL, $recept = NULL, $gerecht = NULL, $keuken = NULL, $season = NULL, $gelegenheid = NULL, $livelinkID) {
  $result = array();
  if ($arg == "filters") {
    $result = _yunomi_rest_recipe_filters();
    return $result;
  }
  elseif ($arg == "search") {
    $result = _yunomi_rest_recipe_search($query, $moment, $menu, $preparation, $recept, $gerecht, $keuken, $season, $gelegenheid);
  }
  elseif ($arg == "product") {
    $result = _yunomi_rest_recipe_product($livelinkID);
  }
  elseif (is_numeric($arg)) {
    $result = _yunomi_rest_recipe_livelink($arg);
  }
  else {
    return services_error( t("No results found."), 406);
  }
  
  return $result;
}

/**
 * Returns the list of brands which have test and tells.
 * 
 * @return
 *   An array of comment objects.
 */
function _yunomi_rest_recipe_filters() {

  $results = array();
  
  // Gather a structured array of filters.
  $results['menu'] = ll_recipe_get_menugang_options();
  $results['gerecht'] = ll_recipe_get_gerecht_options();
  $results['keuken'] = ll_recipe_get_keuken_options();
  $results['season'] = ll_recipe_get_seizoen_options();
  $results['gelegenheid'] = ll_recipe_get_gelegenheid_options();
  $results['moment'] = ll_recipe_get_momenvandedag_options();
  $results['preparation'] = ll_recipe_get_preparation_options();
  
  foreach ($results as $key => $value) {
    foreach ($value as $optionkey => $optionvalue) {
      if(empty($optionkey)) {
        unset($results[$key][$optionkey]);
        $results[$key]['_label'] = $optionvalue;
      } else {
        // cleanup of encoded characters
        unset($results[$key][$optionkey]);
        $results[$key][_remove_accents($optionkey)] = $optionvalue;
      }
    }
  }
  
  return $results;
  
}


/**
 * Returns the array of livelink search results.
 * 
 * @return
 *   An array of livelink search result.
 */
function _yunomi_rest_recipe_search($query, $moment, $menu, $preparation, $recept, $gerecht, $keuken, $season, $gelegenheid) {
  $results = array();
  // Adds backwards compatability with regression fixed in #1083242
  global $user;
  global $language;
  
  if( helper_get_site_country() == 'nl' ) {
    $prefix = 'NL';
  }
  else {
    if($language->language == 'nl') {
      $prefix = 'BN';
    }
    else {
      $prefix = 'BF';
    }
  }
  $search_values['advancedquery'] = 'true';
  
  if (isset($query) && !empty($query)) {
    $search_values['region_OTDoc'] = helper_remove_accents($query);
  }
  if (isset($moment) && !empty($moment)) {
    $search_values['region_Recept ' . $prefix .':Moment van de dag'] = $moment;
  }
  if (isset($menu) && !empty($menu)) {
    $search_values['region_Recept ' . $prefix .':Menugang'] = $menu;
  }
  if (isset($preparation) && !empty($preparation)) {
    $search_values['region_Recept ' . $prefix .':Bereidingstijd (min)'] = $preparation;
  }
  if (isset($preparation) && !empty($preparation)) {
    $search_values['regionmodifier_region_Recept ' . $prefix .':Bereidingstijd (min)'] = "qlrange";
  }
  if (isset($gerecht) && !empty($gerecht)) {
    $search_values['region_Recept ' . $prefix .':Gerecht'] = $gerecht;
  }  
  if (isset($keuken) && !empty($keuken)) {
    $search_values['region_Recept ' . $prefix .':Keuken'] = $keuken;
  }  
  if (isset($season) && !empty($season)) {
    $search_values['region_Recept ' . $prefix .':Seizoen'] = $season;
  } 
  if (isset($gelegenheid) && !empty($gelegenheid)) {
    $search_values['region_Recept ' . $prefix .':Gelegenheid'] = $gelegenheid;
  } 
  $search_values['startat'] = 0;

  $results = livelink_fetch_search($search_values);

  // Replace image_id with a formatted version.
  foreach ($results as $key => $value) {
    if (isset($value['image_id']) && !empty($value['image_id'])) {
      unset($results[$key]['image_id']);
      $results[$key]['image_formatted'] = theme('ll_recipe_image', $value['image_id'], $value['name']);
    }
    $results[$key]['livelink_id'] =  $results[$key]['id'];
    unset($results[$key]['id']);
  }
  unset($results['totalhits']);
  
  return array_values($results);
}


/**
 * Returns the livelink recipe with a by a specified ID.
 * 
 * @return
 *   An array of livelink search result.
 */
function _yunomi_rest_recipe_livelink($data_id) {
  $results = array();
  $recipe = ll_recipe_builder($data_id);
  
  // Get all necessary fields.
  $results['nid'] = $recipe->nid;
  $results['livelink_id'] = $recipe->dataid;
  $results['title'] = $recipe->recipe_fields['titel'];
  // Check if we have big image, if not then use small one.
  if(!empty($recipe->recipe_fields['media']['recept_image_groot'])) {
    $results['image'] = $recipe->recipe_fields['media']['recept_image_groot']['url'];
  }
  else {
    $results['image'] = $recipe->recipe_fields['media']['recept_image']['url'];
  }
  if (isset($recipe->recipe_fields['media']['recept_video']) && !empty($recipe->recipe_fields['media']['recept_video'])) {
    $result['bbwid'] = $recipe->recipe_fields['media']['recept_video'];
  }
  $results['ratings'] = $recipe->recipe_fields['rating'];
  $results['comment_count'] = $recipe->comment_count;
  $results['body'] = theme_yunomi_ll_recipe($recipe->recipe_fields);
  $results['brands'] = theme_yunomi_ll_recipe_brands($recipe->recipe_fields);
  // Provide all unformatted fields just for the case we need something from there.
  $results['fields'] = $recipe->recipe_fields;
  
  return $results;
}


/**
 * Loads an array of ratings of the livelink.
 *
 * @param $nid
 * Number. Livelink ID
 * 
 * @return
 * Array. A list of all ratings for the given livelink id.
 */
function _yunomi_rest_recipe_resource_load_ratings($data_id) {
  $result = array();
  $return = livelink_fetch_recipe($data_id);
  
  if(!$return) {
    return services_error( t('Nothing found.'), 406);
  }
  
  $json = json_encode($return);
  $array = json_decode($json,TRUE);
  
  $result = $array['rating'];
  
  return $result;
}


/**
 * Returns the livelink recipe with a by a specified ID.
 * 
 * @return
 *   An array of livelink search result.
 */
function _yunomi_rest_recipe_product($data_id) {
  $results = array();
  $results = yunomi_rest_recipe_get_product_ws_processed_data($data_id);
  
  return $results;
}


/**
 * Update users password.
 *
 * @param $case
 *   Path of the request. We should check on it to prevent posting to different content locations.
 * @param $data
 *   New password to be set for user, must be plain text and not hashed.
 *
 * @return
 *   A succeeded user array with uid and set new pass.
 */
function _yunomi_rest_recipe_resource_post_rating($rid, $data) {
  // Prevent performing post action to unwanted nodes.
  global $user;
  if($user->uid == 0) {
    return services_error( t('You are not logged in'), 401);
  }
  // Adds backwards compatability with regression fixed in #1083242
  $rating = _services_arg_value($data, 'data');
  
  if (!isset($rating['rate']) || empty($rating['rate'])) {
    return services_error( t('Please, provide the rating value.'), 406);
  }

  $form_state['values']['op'] = variable_get('services_user_save_button_resource_update', t('Rate'));
  $form_state['values']['rid'] = $rid;
  $form_state['values']['rating'] = $rating['rate'];

  $ret = drupal_execute('ll_recipe_rate_recipe_form', $form_state, $rid, 397549);
  // Error if needed.
  if ($errors = form_get_errors()) {
    return services_error(implode(" ", $errors), 406, array('form_errors' => $errors));
  }
  
  return $rating['rate'];
}


/**
 * Helper function to retrieve the processed ws data of the ll_recipe product
 *
 * @param string $dataid
 *   The dataid of the recipe
 *
 * @return array 
 *   
 */
function yunomi_rest_recipe_get_product_ws_processed_data($dataid) {
  $data = livelink_fetch_product($dataid);
  $ret = array();
  $ret['type'] = $data['Product_CW']['type'];
  $ret['title'] = $data['Product_CW']['content']['Titel']['value'];
  $img_dataid = $data['Product_CW']['content']['Packshot']['value'];
  $img = theme('ll_recipe_image', $img_dataid, $data['Product_CW']['content']['Titel']['value']);
  $ret['product_img'] = $img;
  $ret['product_content'] = theme('yunomi_ll_recipe_product', $data, $img);
  
  $ret['fields'] = array();
  $ret['fields']['product_img_id'] = $img_dataid;
  $ret['fields']['product_img_url'] = livelink_image_url($img_dataid);
  $ret['fields']['title'] = $data['Product_CW']['content']['Titel']['value'];
  $ret['fields']['body'] = theme('ll_recipe_product_body', $data);

  return $ret;
}

/**
 * Attach posted media for a the given node.
 *
 * @param $nid
 *   Number. Node ID
 * @return
 *   Array. Response array as REST call result.
 */
function _yunomi_rest_recipe_resource_post_media($nid, $file) {
  // Retrieve arguments values.
  $file = _services_arg_value($file, 'file');
  
  global $user;
  $node = node_load($nid);
  
  // If node doesn't exist.
  if(!$node) {
    return services_error( t('Node does not exist.'), 404);
  }
  // If user is not logged in.
  if ($user->uid == 0) {
    return services_error( t('You have to be logged in'), 401);
  }
  // If the file data is empty then bail.
  if (!isset($file['file'])) {
    return services_error( t('No file data received.'), 406);
  }
  // Check if the node type is article.
  if ($node->type !== 'll_recipe') {
    return services_error( t('Not possible to add file to this node type.'), 406);
  }
  // Check if MIME type allowed.
  $extensions = 'jpg jpeg gif png';
  $allowed = explode(" ", $extensions);
  
  if (!in_array(substr($file['filemime'], stripos($file['filemime'], '/') + 1), $allowed) || !in_array(substr($file['filename'], stripos($file['filename'], '.') + 1), $allowed) ) {
    return services_error( t('Unsupported media type.'), 415);
  }
  
  // Check whether we have filename or filepath for the file.
  if ((!isset($file['filename']) || empty($file['filename']))
    && (!isset($file['filepath']) || empty($file['filepath']))) {
    return services_error('No filename and no filepath specified or they are both empty.');
  }

  // Ensure we create new file.
  $file['fid'] = NULL;

  // If filename has not been specified extract it from filepath.
  if (!isset($file['filename']) || empty($file['filename'])) {
    $file['filename'] = trim(basename($file['filepath']), '.');
  }
   
  // If no filepath specified use standard directory.
  if (!isset($file['filepath']) || empty($file['filepath'])) {
    $file['filepath'] = file_directory_path() . '/' . $node->type . '/' . $node->nid . '/' . $file['filename'];
  }

  // Get the directory name for the location of the file:
  $dir = dirname($file['filepath']);
  // Build the destination folder tree if it doesn't already exists.
  // @see http://drupal.org/node/180970
  $dir_array = explode('/', $dir);
  $file_check_directory_array = array();
  foreach ($dir_array as $dir_element) {
    $file_check_directory_array[] = $dir_element;
    $dir_path_element = implode('/', $file_check_directory_array);
    file_check_directory($dir_path_element, FILE_CREATE_DIRECTORY);
  }
  if (!file_check_directory($dir, FILE_CREATE_DIRECTORY)) {
    return services_error("Could not create destination directory for file. " . $dir);
  }
  // Update file data as necessary
  $file['filepath'] = file_destination(file_create_path($file['filepath']), FILE_EXISTS_RENAME);
  $file['filename'] = file_munge_filename(trim(basename($file['filepath']), '.'), $extensions, TRUE);
  $file['filemime'] = file_get_mimetype($file['filename']);
  $destination = file_destination(file_create_path($file['filepath']), FILE_EXISTS_RENAME);

  // Rename potentially executable files, to help prevent exploits.
  if (preg_match('/\.(php|pl|py|cgi|asp|js)$/i', $file['filename']) && (drupal_substr($file['filename'], -4) != '.txt')) {
    $file['filemime'] = 'text/plain';
    $file['filepath'] .= '.txt';
    $file['filename'] .= '.txt';
    // As the file may be named example.php.txt, we need to munge again to
    // convert to example.php_.txt, then create the correct destination.
    $file['filename'] = file_munge_filename($file['filename'], $extensions, TRUE);
    $destination = file_destination(file_create_path($file['filepath']), FILE_EXISTS_RENAME);
  }

  if (!$filename = file_save_data(base64_decode($file['file']), $destination)) {
    return services_error("Could not write file to destination");
  }
  $file['filepath'] = $destination;

  // Update the timestamp to the current time, otherwise the file could
  // get deleted on the next cron run if its status is set to 0.
  $file['timestamp'] = time();
  $file['uid'] = $user->uid;
  $file['filesize'] = filesize($file['filepath']);
  
  // If we made it this far it's safe to record this file in the database.
  drupal_write_record('files', $file);
  
  $file['data']['title'] = $file['title'];
  
  // Now update the node field with our file.
  $node->field_recipe_media[] = $file;
  node_save($node);
  
  return array(
    'fid' => $file['fid'],
    'uri' => services_resource_uri(array('file', $file['fid'])),
    'nid' => $nid,
    'file' => $file['filepath'],
  );
}


/**
 * Loads an array of medias attached to a node.
 *
 * @param $nid
 * Number. Node ID
 * 
 * @return
 *   Array. A list of all medias from the given node
 */
function _yunomi_rest_recipe_resource_load_media($nid) {
  $return = array();
  
  $node = node_load($nid);
  
  if(!$node) {
    watchdog('yunomi_rest_content', '@function - node could not be loaded', array('@function' => '_yunomi_rest_recipe_resource_load_media'), WATCHDOG_ERROR);
    return services_error( t('Internal Server Error.'), 500);
  }
  
  if (!isset($node->field_recipe_media)) {
    return services_error( t('There are no media on given node.'), 404);
  }

  $return = array();
  foreach ($node->field_recipe_media as $file) {
    if (empty($file['fid'])) {
      continue;
    }
    // Get author data.
    $user = user_load($file['uid']);
    $author = array(
      'name' => $user->name,
      'uid' => $user->uid,
      'picture' => $user->picture,
    );
    // Get both variants of image.
    $image = array(
      'original' => $file['filepath'],
      'resized' => imagecache_create_url('to be defined', $file['filepath'], FALSE),
    );
    // Organize return array.
    $return[] = array(
      'fid' => $file['fid'],
      'title' => $file['data']['title'],
      'image'  => $image,
      'author'  => $author,
    );
  }
  
  return $return;
}
