<?php
/**
* @file
* Callbacks for services module resource hooks.
*/


/**
 * Callback for retrieving views resources to display Battle of the week.
 * 
 * @return
 *  Array.
 */
function _yunomi_rest_battle_of_the_week() {
  // Check if the user is logged in
  if(!user_is_logged_in()) {
    return services_error( t('You must be logged in.'), 401);
  }
  // Get a view with an actual battle of the week.
  $view = views_get_view('battle_of_the_week_archive');
  if (!$view) {
    watchdog('yunomi_rest_botw', '@function @name - view could not be found', array('@function' => '_yunomi_rest_battle_of_the_week', '@name' => 'battle_of_the_week_archive'), WATCHDOG_ERROR);
    return services_error( t('Internal Server Error.'), 500);
  }
  
  // Execute the view to get node id. 
  $view->set_display('block_1');
  $view->execute();
  $view_result = $view->result;
  $nid = $view_result[0]->nid;
  
  // Now we have node id, let's return all the data.
  global $user;
  $node = node_load($nid);
  
  // Check if we actually retrieved a node
  if(!$node) {
    watchdog('yunomi_rest_botw', '@function - node could not be loaded', array('@function' => '_yunomi_rest_battle_of_the_week'), WATCHDOG_ERROR);
    return services_error( t('Internal Server Error.'), 500);
  }
  
  // Initialize and fill in the result array.
  $result = array();
  $result['question'] = $node->title;
  
  // Rename + reorder
  $result['options']['option_1']['title'] = $node->field_image_left[0]['data']['title'];       
  $result['options']['option_1']['image'] = $node->field_image_left[0]['filepath'];       
  $result['options']['option_2']['title'] = $node->field_image_right[0]['data']['title'];
  $result['options']['option_2']['image'] = $node->field_image_right[0]['filepath'];
  
  // Deliver stats.
  $stats = battle_of_the_week_get_stats($nid);
  
  // Get rid of unnecessary clutter.
  unset($stats['result_left_padding']);
  unset($stats['result_right_padding']);
  unset($stats['result_left']);
  unset($stats['result_right']);
  
  // Rename the votes keys.
  $stats['option_1_votes'] =  $stats['left_votes'];
  $stats['option_2_votes'] =  $stats['right_votes'];
  
  // Unset fields
  unset($stats['right_votes']);
  unset($stats['left_votes']);
  
  $result['results'] = $stats;
  
  // Check if user already voted.
  $has_played = isset($_SESSION['botw_' . $nid . '_has_played']) || battle_of_the_week_db_select_has_played($user->uid, $nid);
  $has_played = intval($has_played);
  
  $result['voted'] = $has_played;
        
  return $result;
}


/**
 * Callback for accepting the posted vote for Battle of the week.
 *
 * @param $vote
 * String. The side user votes for. Could be wether "r" or "l".
 * 
 * @return
 *  Array.
 */
function _yunomi_rest_botw_save_vote($vote) {
  
  // Check if the user is logged in
  if(!user_is_logged_in()) {
    return services_error( t('You must be logged in.'), 401);
  }
  // Get a view with an actual battle of the week.
  $view = views_get_view('battle_of_the_week_archive');
  if (empty($view)) {
    return services_error( t('Invalid battle ID.'), 406);
  }
  // Execute the view to get node id. 
  $view->set_display('block_1');
  $view->execute();
  $view_result = $view->result;
  $nid = $view_result[0]->nid;
  
  if (!isset($nid)) { return services_error( t('No node id found'), 500); } 
  
  // Check if user already voted.
  global $user;
  $has_played = isset($_SESSION['botw_' . $nid . '_has_played']) || battle_of_the_week_db_select_has_played($user->uid, $nid);
  $has_played = intval($has_played);
  // Check if user already played.
  if ($has_played) {
    return services_error( t('You have already voted for this battle.'), 409);
  }
  // Save the vote.
  battle_of_the_week_insert_vote($user->uid, $nid, $vote);
  $results = battle_of_the_week_get_stats($nid, TRUE);
  // Get stats and return them.
  $left = $results['left_votes'];
  $right = $results['right_votes'];
  // Create a new object to be returned.
  $data = new stdClass();
  $data->left = intval($left);
  $data->right = intval($right);
 
  return $data;
}


/**
 * Callback for accepting the posted vote for Battle of the week.
 *
 * @param $vote
 * String. The side user votes for. Could be wether "r" or "l".
 * 
 * @return
 *  Array.
 */
function _yunomi_rest_botw_save_deletevote() {
  
  // Check if the user is logged in
  if(!user_is_logged_in()) {
    return services_error( t('You must be logged in.'), 401);
  }
  // Get a view with an actual battle of the week.
  $view = views_get_view('battle_of_the_week_archive');
  if (empty($view)) {
    return services_error( t('Invalid battle ID.'), 406);
  }
  // Execute the view to get node id. 
  $view->set_display('block_1');
  $view->execute();
  $view_result = $view->result;
  $nid = $view_result[0]->nid;
  
  if (!isset($nid)) { return services_error( t('No node id found'), 500); } 
  
  // Check if user already voted.
  global $user;
  $has_played = isset($_SESSION['botw_' . $nid . '_has_played']) || battle_of_the_week_db_select_has_played($user->uid, $nid);
  $has_played = intval($has_played);
  // Check if user already played.
  if (!$has_played) {
    return services_error( t('You did not vote for this battle.'), 409);
  }
  // Save the vote.
  battle_of_the_week_delete_vote($user->uid, $nid);
  $results = battle_of_the_week_get_stats($nid, TRUE);
  // Get stats and return them.
  $left = $results['left_votes'];
  $right = $results['right_votes'];
  // Create a new object to be returned.
  $data = new stdClass();
  $data->left = intval($left);
  $data->right = intval($right);
 
  return $data;
}