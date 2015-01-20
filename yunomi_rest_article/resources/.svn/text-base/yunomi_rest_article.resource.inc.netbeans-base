<?php
/**
* @file
* Callbacks for services module resource hooks.
*/

/**
 * Returns the formatted results of a node_load() for the specified node.
 *
 * @param $nid
 *   NID of the node we want to return.
 * @return
 *   Formatted node object array or FALSE if not found.
 *
 * @see node_load()
 */
function _yunomi_rest_article_retrieve($nid) {
  $result = array();
  $node = node_load($nid);
  
  if ($node && $node->type == 'article') {
    $result = _yunomi_rest_format_article($node);
    return $result;
  }
  return services_error( t('No results found'), 404);
}