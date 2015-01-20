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
function _yunomi_rest_testtell_retrieve($arg) {
  $result = array();
  
  if ($arg == "brands") {
    $result = _yunomi_rest_testtell_brands();
  }
  else {
    $node = node_load($arg);
    if (!$node && $node->type !== 'test_tell') {
      return services_error( t('No results found'), 404);
    }
    $node->rating = votingapi_select_single_result_value(array('content_type' => 'node', 'content_id' => $node->nid, 'tag' => 'average', ));
    $node->questions = testandtell_get_questions($node->nid);
    $result = _yunomi_rest_format_testtells($node);
  }
  
  return $result;
}

/**
 * Returns the comments of a specified node.
 *
 * @param $nid
 *   Unique identifier for the node.
 * @param $count
 *   Number of comments to return.
 * @param $start
 *   Which comment to start with. if present, $start and $count are used together
 *   to create a LIMIT clause for selecting comments. This could be used to do paging.
 * @return
 *   An array of comment objects.
 */
function _yunomi_rest_testtell_resource_load_node_comments($nid, $count = 0, $start = 0) {
  $comments = array();
  $limit = ((int)$count > 0 ? ' LIMIT ' . (int)$start . ', ' . (int)$count . ' ' : '');
  $result = db_query("SELECT a.cid, a.pid, a.comment, a.name, a.uid, b.created, b.picture FROM {comments} as a INNER JOIN {users} as b ON b.uid = a.uid WHERE a.nid = %d AND a.status = %d ORDER BY a.thread DESC" . $limit, $nid, COMMENT_PUBLISHED);
  $votes = testandtell_get_useraverages($nid);
  
  while ($comment = db_fetch_array($result)) {
    $comment['rating'] = 0;
    
    if(isset($votes[$comment['uid']])) {
      $comment['rating'] = $votes[$comment['uid']];
    }
    
    $comments[] = array(
      'cid' => $comment['cid'],
      'parent' => $comment['pid'],
      'comment' => $comment['comment'],
      'rating' => ceil($comment['rating']),  
      'author' => $comment['name'],
      'picture' =>  $comment['picture'],
      'created' => $comment['created'],  
    );    
  }
  //  TODO Add default user pic if doesn't exist
  return $comments;
}

/**
 * Returns the list of brands which have test and tells.
 * 
 * @return
 *   An array of comment objects.
 */
function _yunomi_rest_testtell_brands() {
  $result = array();
  
  // Do a custom DB query.
  $sql = "SELECT tdata.name FROM {node} node";
  $sql .= " INNER JOIN  {term_node} term ON node.nid = term.nid";
  $sql .= " INNER JOIN  {term_data} tdata ON term.tid = tdata.tid";
  $sql .= " WHERE node.type =  'test_tell'";
  $sql .= " AND tdata.vid =16";
  $sql .= " GROUP BY tdata.tid";
  $db_result = db_query($sql);
  
  while ($res = db_fetch_array($db_result)) {
      $result[_yunomi_rest_brands_checkterm($res['name'])] = $res['name'];
    }
  
  return $result;
}

/**
 * Callback for posting comment to testtell node.
 *
 * @return
 * Formatted array or FALSE if not found.
 */
function _yunomi_rest_testtell_resource_post_comment($nid, $comment) {
  // Check if there's such a node.
  $sql = "SELECT count(*) FROM {node} WHERE nid = %d";
  $count = db_result(db_query($sql, $nid));
  if ($count == 0) {
    return services_error( t('Node does not exist.'), 404);
  }
  // Adds backwards compatability with regression fixed in #1083242
  $comment = _services_arg_value($comment, 'comment');
  $comment['nid'] = $nid;
  if (empty($comment['nid'])) {
    return services_error( t('A nid must be provided'), 406);
  }
  
  // Setup form_state
  $form_state = array();
  $form_state['values'] = $comment;
  $form_state['values']['op'] = variable_get('services_comment_save_button_resource_create', t('Save'));

  $comment_empty = array(
    'nid' => $comment['nid'],
    'cid' => NULL,
  );

  if (!empty($comment['pid'])) {
    $comment_empty['pid'] = $comment['pid'];
  }
  
  global $user;
  $vote = votingapi_select_votes(array('content_type' => 'node', 'content_id' => $nid, 'uid' => $user->uid, 'tag' => 'average', ));
  if (count($vote) > 0) {
      return services_error( t('You have already voted and commented this.'), 406);
  }
  
  $ret = drupal_execute('testandtell_form', $form_state, $comment_empty);
  
  if ($errors = form_get_errors()) {
    return services_error(implode(" ", $errors), 406, array('form_errors' => $errors));
  }

  // Load latest created comment.
  $recent_comments = comment_get_recent(1);
  $cid = $recent_comments[0]->cid;

  return array(
    'cid' => $cid,
    'uri' => services_resource_uri(array('comment', $cid)),
  );

  return $comment;
}

/**
 * Attach posted media for a the given node.
 *
 * @param $nid
 *   Number. Node ID
 * @return
 *   Array. Response array as REST call result.
 */
function _yunomi_rest_testtell_resource_post_media($nid, $file) {
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
  if ($node->type !== 'test_tell') {
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
  $node->field_test_tell_media[] = $file;
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
function _yunomi_rest_testtell_resource_load_media($nid) {
  $return = array();
  $node = node_load($nid);
  
  if(!$node) {
    watchdog('yunomi_rest_testtell', '@function - node could not be loaded', array('@function' => '_yunomi_rest_testtell_resource_load_media'), WATCHDOG_ERROR);
    return services_error( t('Internal Server Error.'), 500);
  }
  
  if (!isset($node->field_test_tell_media)) {
    return services_error( t('There are no media on given node.'), 404);
  }

  $return = array();
  foreach ($node->field_test_tell_media as $file) {
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
