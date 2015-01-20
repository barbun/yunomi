<?php
/**
* @file
* Callbacks for services module resource hooks.
*/


/**
 * Callback for retrieving resources depending on the case. Can be view or just a custom option for articles.
 *
 * @return
 * Formatted or empty array if not found.
 */
function _yunomi_rest_articles_view($case, $query, $qid, $offset = 0, $limit = 10, $term = NULL ) {
  $result = array();
  // Check the case we've got. 
  if($case == 'recent') {
    $view_name = "latest_articles_search_like";
  
    // Set some basic parameters to execute the view.
    $display_id = 'default';
    $args = array($term);
    $return_type = 'view';
    $filters = array();
    
    $view = _yunomi_rest_views_retrieve($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);
    
    // For each result node perform fields formatting.
    foreach ($view->result as $key => $value) {
      $result[] = _yunomi_rest_format_article(node_load($value->nid));
    }
  }
  // Check the case we've got.
  elseif ($case == 'mostcommented') {
    $view_name = "thema_badges";
  
    // Set some basic parameters to execute the view.
    $display_id = 'block_2';
    $return_type = 'view';
    $filters = array();
    
    // Check if we've passed term argument.
    if ($term && !empty($term)) {
      $args = array($term);
      $view = _yunomi_rest_views_retrieve($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);
    }
    else {
      $args = array();
      $view = _yunomi_rest_views_retrieve_ignore_tid($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);
    }
     // For each result node perform fields formatting.
    foreach ($view->result as $key => $value) {
      $result[] = _yunomi_rest_format_article(node_load($value->nid));
    }
  }
  // Check the case we've got.
  elseif ($case == 'mostviewed') {
    $view_name = "thema_badges";
  
    // Set some basic parameters to execute the view.
    $display_id = 'block_1';
    $return_type = 'view';
    $filters = array();
    // Check if we've passed term argument.
    if ($term && !empty($term)) {
      $args = array($term);
      $view = _yunomi_rest_views_retrieve($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);
    }
    else {
      $args = array();
      $view = _yunomi_rest_views_retrieve_ignore_tid($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);
    }
    // Run the special function that will ignore tid argument that is set as necessary in the view.
    // For each result node perform fields formatting.
    foreach ($view->result as $key => $value) {
      $result[] = _yunomi_rest_format_article(node_load($value->nid));
    }
  }
  // Check the case we've got.
  elseif ($case == 'queue') {
    $view_name = "queue_content";
    // Set some basic parameters to execute the view.
    $args = array($qid);
    $display_id = 'default';
    $return_type = 'view';
    $filters = array();
    
    $view = _yunomi_rest_views_retrieve($view_name, $display_id, $args, $offset, $limit, $return_type, $filters);
    // For each result node perform fields formatting.
    foreach ($view->result as $key => $value) {
      $result[] = _yunomi_rest_format_article_queue(node_load($value->nid));
    }
  }
  // Check the case we've got.
  elseif ($case == 'search') {
    $results = $_results = array();
    // Define the type of the content we are searching in.
    $type = "article";
    
    if($type){
      $_GET['filters'] = 'type:' . $type;
    }
    
    if(!isset($_GET['page']) || empty($_GET['page'])) {
      $_GET['page'] = 0;
    }
    if(isset($_GET['offset']) && !empty($_GET['offset'])) {
      $_GET['page'] = $_GET['offset'];
    }
    
    // Apply parameters and run apacheSolr search.
    global $conf;
    $conf['apachesolr_rows'] = $limit;
    $results = apachesolr_search_search($op = 'search', $query);
    
    if (isset($results) && is_array($results) && count($results)) {
      foreach($results as $result) {
        $node = node_load($result['node']->nid);
        if ($node && $node->type == 'article') {
          $_results[] = _yunomi_rest_format_article($node);
        }
      }
      
      return $_results;
      
    }
    else {
      return services_error( t('No results found'), 404);
    }
  }
  else {
    return services_error(t('No results found'), 404);
  }
  
  return $result;
}
