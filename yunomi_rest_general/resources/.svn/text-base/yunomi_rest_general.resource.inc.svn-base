<?php
/**
* @file
* Callbacks for services module resource hooks.
*/

/**
* Callback for retrieving settings array.
*
* @return array
*/
function _yunomi_rest_general_settings_all() {
  global $user; 
  
  $form = yunomi_rest_values_settings_form();
  $settings = array();
  
  $settings['uid'] = $user->uid;
  
  $settings['hubs'] = _yunomi_rest_settings_get_hubs();
  
  // Go through all the settings fields and build a result based on it.
  if ($form['settings']['#type'] == "fieldset") {
    foreach ($form['settings'] as $key => $value) {
      if (is_array($form['settings'][$key])) {
        $settings[$value['#title']] = $form['settings'][$key]['#default_value']; 
      }
    }
  }
  else {
    foreach ($form['settings'] as $key => $value) {
      $settings[$value['#title']] = $form['settings'][$key]['#default_value']; 
    }  
  }
  
  return $settings;
}


/**
* Callback for retrieving labels array.
*
* @return array
*/
function _yunomi_rest_general_labels_all() {
  $form = yunomi_rest_values_labels_form();
  $labels = array();
  
  // Go through all the label fields and build a result based on it.
  if ($form['labels']['#type'] == "fieldset") {
    foreach ($form['labels'] as $key => $value) {
      if (is_array($form['labels'][$key])) {
        $labels[$value['#title']] = $form['labels'][$key]['#default_value']; 
      }
    }
  }
  else {
    foreach ($form['labels'] as $key => $value) {
      $labels[$value['#title']] = $form['labels'][$key]['#default_value']; 
    }  
  }
 
  return $labels;
}

/**
* Callback for retrieving brands array.
*
* @return array
*/
function _yunomi_rest_general_brands() {

  $brands = badges_get_brand_badges();

  foreach ($brands as $tid => $brand) {
    if(!empty($brand->badge_menu_link_url) || !empty($brand->badge_menu_link_url_fr)) {
      unset($brands[$tid]);
    }
  }
 
  return array_values($brands);

}


/**
 * Create an abuse report.
 *
 * @param $case
 *   Path of the request. We should check on it to prevent posting to different content locations.
 * @param $data
 *   New password to be set for user, must be plain text and not hashed.
 *
 * @return
 *   A succeeded user array with uid and set new pass.
 */
function _yunomi_rest_general_abuse_create($data) {
  $abuse = array();
  $abuse = _services_arg_value($data, 'data');
  
  // Check if there's such a node.
  $sql = "SELECT count(*) FROM {node} WHERE nid = %d";
  $count = db_result(db_query($sql, $abuse['nid']));
  if ($count == 0) {
    return services_error( t('Node does not exist.'), 404);
  } 
  // Prevent performing post action to unwanted nodes.
  global $user;
  
  if($user->uid == 0) {
    return services_error( t('You are not logged in'), 401);
  }
  // Create a new abuse report node.
  $node = node_load($abuse['nid']);
  $abuse_node = new stdClass();
  $abuse_node->type = "abuse_report";
  $abuse_node->uid = $user->uid;
  $abuse_node->complaint_nid = $abuse['nid'];
  $abuse_node->complaint_url = url($node->path, $options = array('absolute' => TRUE));
  if (!empty($abuse['message'])) {
    $abuse_node->body = $abuse['message'];
  }
  else {
    $abuse_node->body = "";
  }
  $abuse_node = node_submit($abuse_node);
  node_save($abuse_node);
  
  // Insert abuse report node to DB.
  abuse_insert($abuse_node);
  
  return $abuse;
}


/**
* Callback for retrieving banners list.
*
* @return array
*/
function _yunomi_rest_general_retrieve_banners($op) {
  $list = array();
  
  if ($op == "list") {
    $list = _banner_get_banner_mobile_raw();
  }
  
  return $list;
}


/*
 * Helper function to get hubs list for settings service.
 * 
 * param $vid
 * ID of the vocabulary
 * 
 * return
 * Array of hubs.
 * 
 */
function _yunomi_rest_settings_get_hubs($vid) {
  $hubs = badges_get_hubs();
  return $hubs;
}

/**
* Callback for retrieving taxonomy tree.
*
* @return array
*/
function _yunomi_rest_general_taxonomy() {
  $results = array();
  
  $sql = "SELECT data.tid, data.name, badges.badge_image_path FROM {term_data} data";
  $sql .= " INNER JOIN {term_hierarchy} hierarchy ON data.tid = hierarchy.tid";
  $sql .= " LEFT JOIN {term_badges} badges ON data.tid = badges.tid";
  $sql .= " WHERE hierarchy.parent = %d";
  $sql .= " AND data.vid = 10";
  
  $result = db_query($sql, 0);
  
  while ($main = db_fetch_array($result)) {
    
    $parent = array(
      'name' => $main['name'],
      'tid' => $main['tid'],
      'image' => $main['badge_image_path'] <> '' ? $main['badge_image_path'] : '',
      'type' => 'hub',
    ); 
    
    $result2 = db_query($sql, $main['tid']);
    
    while ($second = db_fetch_array($result2)) {
      $parent['children'][] = array(
        'name' => $second['name'],
        'tid' => $second['tid'],
        'image' => $second['badge_image_path'] <> '' ? $second['badge_image_path'] : '',
        'type' => 'badge',
      );
      
    }
    
    $results[] = $parent;
    
  }
  
  if (empty($results)) {
    return services_error( t('No results found.'), 404);
  }
  
  return $results;
}