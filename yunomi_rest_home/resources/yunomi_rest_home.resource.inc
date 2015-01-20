<?php
/**
* @file
* Callbacks for services module resource hooks.
*/


/**
 * Callback for retrieving homepage resources.
 *
 * @return
 *  Array. The views return.
 */
function _yunomi_rest_home() {
  // GET 4 RECENT ARTICLES.
  $return['articles'] = _yunomi_rest_home_recent_articles();
  // GET OPENHARTIG
  $return['openhartig'] = _yunomi_rest_home_openhartig();
  // GET 4 RECIPES.
  $return['recipes'] = _yunomi_rest_home_recent_recipes();
  
  return $return;
}

/*
 * Helper function to get openhartig.
 * 
 * @return
 * Array. Formatted result of DB query.
 */
function _yunomi_rest_home_openhartig() {
  $result = array();  
  
  // TO DO check queue
  // Build the query
  $sql = "SELECT node.nid AS nid";
  $sql .= " FROM drupal_be_node node";
  $sql .= " INNER JOIN drupal_be_queue_items queue_items ON node.nid = queue_items.nid";
  $sql .= " INNER JOIN drupal_be_queue queue ON queue.queueid = queue_items.queueid";
  $sql .= " WHERE node.status = 1";
  $sql .= " AND node.language = 'nl'";
  $sql .= " AND queue.queue_name = 'Woman of the day'";
  $sql .= " AND queue_items.displaydate <= NOW()";
  $sql .= " ORDER BY UNIX_TIMESTAMP( queue_items.displaydate ) DESC LIMIT 1";
  $db_result = db_query($sql);
  
  $item = db_fetch_array($db_result);
  $result = _yunomi_rest_format_article(node_load($item['nid']));
  
  return $result;
}

/*
 * Helper function to get recent articles.
 * 
 * @return
 * Array. Formatted result of views query.
 */
function _yunomi_rest_home_recent_articles() {
  $result = array();
  $limit = array();
  
  $view_name = "frontpage_content";
  $display_id = "block_3";
  $limit = 4;
  $offset = 0;
  $return_type = "view";
  
  $view = _yunomi_rest_views_retrieve($view_name, $display_id, array(), $offset, $limit, $return_type, array());
  
  foreach ($view->result as $key => $value) {
    // Add to result array.
    $result[] = _yunomi_rest_format_article(node_load($value->nid));
  }
  
  return $result;
}

/*
 * Helper function to get recent recipes.
 * 
 * @return
 * Array. Formatted result of views query.
 */
function _yunomi_rest_home_recent_recipes() {
  $result = array();
  $limit = array();
  
  $view_name = "recipes_of_the_week";
  $display_id = "block_1";
  $limit = 4;
  $offset = 0;
  $return_type = FALSE;
  
  $view = _yunomi_rest_views_retrieve($view_name, $display_id, array(), $offset, $limit, $return_type, array());
  
  if(empty($view)) {
    return $result;
  }
  
  foreach ($view as $key => $value) {
    // Add to result array.
    $result[] = _yunomi_rest_home_recipe_livelink(ll_recipe_get_mapped_dataid($value->nid));
  }
  
  return $result;
}


/**
 * Returns the livelink recipe with a by a specified ID.
 * 
 * @return
 *   An array of livelink search result.
 */
function _yunomi_rest_home_recipe_livelink($data_id) {
  $results = array();
  $recipe = ll_recipe_builder($data_id);
  
  // Get all necessary fields.
  $results['nid'] = $recipe->nid;
  $results['livelink_id'] = $recipe->dataid;
  $results['name'] = $recipe->recipe_fields['titel'];
  // Check if we have big image, if not then use small one.
  if(!empty($recipe->recipe_fields['media']['recept_image_groot'])) {
    $results['image_url'] = $recipe->recipe_fields['media']['recept_image_groot']['url'];
  }
  else {
    $results['image_url'] = $recipe->recipe_fields['media']['recept_image']['url'];
  }
  $results['bereiding'] = $recipe->recipe_fields['bereiding'];
  $results['rating'] = $recipe->recipe_fields['rating'];
  $results['comment_count'] = $recipe->comment_count;
  $results['body'] = theme_yunomi_ll_recipe($recipe->recipe_fields);
  $results['brands'] = theme_yunomi_ll_recipe_brands($recipe->recipe_fields);
  // Provide all unformatted fields just for the case we need something from there.
  $results['fields'] = $recipe->recipe_fields;
  
  return $results;
}