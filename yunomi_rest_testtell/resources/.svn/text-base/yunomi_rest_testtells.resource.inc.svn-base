<?php
/**
* @file
* Callbacks for services module resource hooks.
*/

/**
 * Callback for retrieving views resources for testtells.
 *
 * @return
 * Formatted array or FALSE if not found.
 */
function _yunomi_rest_testtells($brand) {
  $result = array();
  // Get the view that contains all testtells.
  if (!empty($brand)) {
    $tid = taxonomy_get_term_by_name($brand);
    $filters['tid'] = $tid[0]->tid;
  }
  $offset = 0;
  $limit = 0;
  $view_name = "test_and_tell";
  $display_id = "page_1";
  
  $view = _yunomi_rest_views_retrieve($view_name, $display_id, array(), $offset, $limit, $return_type = 'view', $filters);
  
  foreach ($view->result as $key => $obj) {
    $node = node_load($obj->nid);
    $node->rating = votingapi_select_single_result_value(array('content_type' => 'node', 'content_id' => $node->nid, 'tag' => 'average', ));
    $result[] = _yunomi_rest_format_testtells($node);
  }
  
  return $result;
}
