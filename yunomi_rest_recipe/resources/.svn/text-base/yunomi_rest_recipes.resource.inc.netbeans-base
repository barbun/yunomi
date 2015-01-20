<?php
/**
* @file
* Callbacks for services module resource hooks.
*/

/**
 * Callback for retrieving views resources for articles.
 *
 * @return
 * Formatted array or FALSE if not found.
 */
function _yunomi_rest_recipes_week($view_name, $offset = 0, $limit = 10) {
  $result = array();
  // Get the recent articles.
  if($view_name == 'week') {
    $view_name = "recipes_of_the_week";
    // Set some parameters to execute the view.
    $display_id = 'block_1';
    $args = array();
    $return_type = 'view';
    $limit = 7;
    $filters = array();

    $view = _yunomi_rest_views_retrieve($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);

    foreach ($view->result as $key => $value) {
      $result[] = _yunomi_rest_format_recipes(node_load($value->nid));
    }

  }
  elseif ($view_name == 'mostviewed') {
    $view_name = "thema_badges";
    // Set some parameters to execute the view.
    $display_id = 'block_1';
    $return_type = 'view';
    $filters = array('type' => 'll_recipe');
    
    $view = _yunomi_rest_views_retrieve_ignore_tid($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);
  
    foreach ($view->result as $key => $value) {
      $result[] = _yunomi_rest_format_recipes(node_load($value->nid));
    }
  }
  else {
    return services_error( t('No results found.'), 404);
  }
  
  return $result;
}
