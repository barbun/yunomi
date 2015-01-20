<?php
/**
 * @file
 * This file will define the resources for dealing with the user object
 */

/**
 * Get user details.
 *
 * @param $uid
 *   UID of the user to be loaded.
 *
 * @return
 *   A user object.
 *
 * @see user_load()
 */
function _yunomi_rest_member_resource_retrieve($uid) {
  $account = user_load($uid);
  if (!$account && empty($account)) {
    return services_error( t('There is no user with such ID.'), 404);
  }
  
  services_remove_user_data($account);
  // Everything went right.
  return $account;
}

/**
 * Create a new user.
 *
 * This function uses drupal_execute() and as such exepects all input to match
 * the submitting form in question.
 *
 * @param $account
 *   A object containing account information. The $account object should
 *   contain, at minimum, the following properties:
 *     - name (user name)
 *     - mail (email address)
 *     - pass (plain text unencrypted password)
 *
 *   These properties can be passed but are optional
 *     - status (0 for blocked, otherwise will be active by default)
 *     - notify (1 to notify user of new account, will not notify by default)
 *
 *  Roles can be passed in a roles property which is an associative
 *  array formatted with '<role id>' => '<role id>', not including
 *  the authenticated user role, which is given by default.
 *
 * @return
 *   The user object of the newly created user.
 */
function _yunomi_rest_member_resource_create($account) {
  // Basic check.
  global $user;
  if ($user->uid) {
    // user is already logged in
    return services_error( t('Already logged in as !user.', array('!user' => $user->name)), 406);
  }
  
  // Adds backwards compatability with regression fixed in #1083242
  $account = _services_arg_value($account, 'account');
  // Load the required includes for saving profile information
  // with drupal_execute().
  module_load_include('inc', 'user', 'user.pages');
  
  // register a new user
  $form_state = array();
  $form_state['values'] = $account;
  $form_state['values']['pass'] = array(
    'pass1' => $account['pass'],
    'pass2' => $account['pass'],
  );
  $form_state['values']['op'] = variable_get('services_user_create_button_resource_create', t('Create new account'));

  // execute the register form
  drupal_execute('user_register', $form_state);

  // Error if needed.
  if ($errors = form_get_errors()) {
    return services_error(implode(" ", $errors), 406, array('form_errors' => $errors));
  }
  else {
    $user = array('uid' => $form_state['user']->uid);
    return $user;
  }
}


/**
 * Login a user using the specified credentials.
 *
 * Note this will transfer a plaintext password.
 *
 * @param $username
 *   Username to be logged in.
 * @param $password
 *   Password, must be plain text and not hashed.
 *
 * @return
 *   A valid session object.
 */
function _yunomi_rest_member_resource_login($username, $password) {
  global $user;

  if ($user->uid) {
    // user is already logged in
    return services_error(t('Already logged in as !user.', array('!user' => $user->name)), 406);
  }
  // Here goes our patch.
  if (valid_email_address($username)) {
    // Make a DB query to retrieve user name.
    $sql = "SELECT name FROM {users} WHERE mail = '%s' LIMIT 1";
    $db_result = db_result(db_query($sql, $username));
    if ($db_result) {
      $username = $db_result;  
    }
  }
  
  user_authenticate(array('name' => $username, 'pass' => $password));

  if (isset($user->uid) && $user->uid) {
    // Regenerate the session ID to prevent against session fixation attacks.
    sess_regenerate();

    $return = new stdClass();
    $return->sessid = session_id();
    $return->session_name = session_name();

    services_remove_user_data($user);

    $return->user = $user;

    return $return;
  }
  session_destroy();
  return services_error( t('Wrong username or password.'), 401);
}

/**
 * Logout the current user.
 */
function _yunomi_rest_member_resource_logout() {
  global $user;

  if (!$user->uid) {
    // User is not logged in
    return services_error( t('User is not logged in.'), 406);
  }

  watchdog('user', 'Session closed for %name.', array('%name' => theme('placeholder', $user->name)));

  // Destroy the current session:
  session_destroy();
  $null = NULL; // Only variables can be passed by reference workaround
  user_module_invoke('logout', $null, $user );

  // Load the anonymous user
  $user = drupal_anonymous_user();

  return TRUE;
}

/**
 * Return an array of optionally paged nids based on a set of criteria.
 *
 * An example request might look like
 *
 * http://domain/endpoint/user?fields=uid,name,mail&parameters[uid]=1
 *
 * This would return an array of objects with only uid, name and mail defined,
 * where uid = 1.
 *
 * @param $page
 *   Page number of results to return (in pages of 20).
 * @param $fields
 *   The fields you want returned.
 * @param $parameters
 *   An array containing fields and values used to build a sql WHERE clause
 *   indicating items to retrieve.
 * @param $page_size
 *   Integer number of items to be returned.
 * @return
 *   An array of user objects.
 *
 * @see _node_resource_index()
 */
function _yunomi_rest_member_resource_index($page, $fields, $parameters, $page_size) {
  if (!user_access('administer users')) {
    $parameters['active'] = 1;
  }
  $query = services_resource_build_index_query('users', 'u.created DESC', $page, $fields, $parameters, 'u', 'uid', $page_size, 'user');

  $results = array();
  while ($user = db_fetch_object($query)) {
    $results[] = $user;
  }
  return services_resource_build_index_list($results, 'user', 'uid');
}

/**
 * Access check callback for user resource.
 */
function _yunomi_rest_member_resource_access($op = 'view', $args = array()) {
  // Adds backwards compatability with regression fixed in #1083242
  $args[0] = _services_access_value($args[0], array('account', 'data'));
  // Check if the user exists if appropriate.
  if ($op != 'create' && $op != 'register' ) {
    $account = user_load($args[0]);
    if (!$account) {
      return services_error(t('There is no user with ID @uid.', array('@uid' => $args[0])), 406);
    }
  }
  global $user;
  switch ($op) {
    case 'view':
        return user_view_access($account);
    case 'update':
      return ($user->uid == $account->uid || user_access('administer users'));
    case 'create':
      return user_access('administer users');
    case 'register':
      //Verify user is not logged in, and verify that visitors can create accounts.
      if (!$user->uid && variable_get('user_register', 1) != 0) {
        return TRUE;
      }
      else {
        return FALSE;
      }
    case 'delete':
      return user_access('administer users');
  }

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
function _yunomi_rest_member_resource_custom_update($case, $data) {
  // Prevent performing post action to unwanted nodes.
  global $user;
  
  if($user->uid == 0) {
    return services_error( t('You are not logged in'), 401);
  }
  if($case == 'password' ) {
    // Adds backwards compatability with regression fixed in #1083242
    $account = _services_arg_value($data, 'data');
    $account['uid'] = $user->uid;
    $account['email'] = $user->mail;
    // In case we haven't passed any password value.
    if (!isset($account['password']) || empty($account['password'])) {
      return services_error( t('Please, provide the password value.'), 406);
    }

    // Load the required includes for saving profile information
    // with drupal_execute().
    module_load_include('inc', 'user', 'user.pages');

    // Any logged in user is by default authenticated,
    // and leaving this role set in the user's roles array
    // causes big problems because of a FAPI hack that controls
    // this checkbox on the user create and edit form (and thus
    // causes problems with drupal_execute()). Therefore we just
    // force it to 0 here.
    if (isset($account['roles'][2])) {
      $account['roles'][2] = 0;
    }

    // If a profile category was passed in, use it. Otherwise default
    // to 'account' (for saving core user data.)
    $category = 'account';
    if (isset($account['category'])) {
      $category = $account['category'];
      unset($account['category']);
    }

    // Drop any passed in values into the $account var. Anything
    // unused by the form just gets ignored.
    foreach ($account as $key => $value) {
      $form_state['values'][$key] = $value;
    }

    $form_state['values']['op'] = variable_get('services_user_save_button_resource_update', t('Save'));
    $form_state['values']['_category'] = $category;
    $form_state['values']['_account'] = (object)$account;
    if (isset($account['password'])) {
      $form_state['values']['password'] = array(
        'pass1' => $account['password'],
        'pass2' => $account['password'],
      );
    }

    $ret = drupal_execute('userinfos_change_password_form', $form_state, $user, $category);
    // Error if needed.
    if ($errors = form_get_errors()) {
      return services_error(implode(" ", $errors), 406, array('form_errors' => $errors));
    }

    services_remove_user_data($account);

    return $account;
  }
  elseif($case == 'email' ) {
    // Adds backwards compatability with regression fixed in #1083242
    $account = _services_arg_value($data, 'data');
    $account['uid'] = $user->uid;
    // In case we haven't passed any password value.
    if (!isset($account['email']) || empty($account['email'])) {
      return services_error( t("Please, provide the email value."), 406);
    }

    // Load the required includes for saving profile information
    // with drupal_execute().
    module_load_include('inc', 'user', 'user.pages');

    // Any logged in user is by default authenticated,
    // and leaving this role set in the user's roles array
    // causes big problems because of a FAPI hack that controls
    // this checkbox on the user create and edit form (and thus
    // causes problems with drupal_execute()). Therefore we just
    // force it to 0 here.
    if (isset($account['roles'][2])) {
      $account['roles'][2] = 0;
    }

    // If a profile category was passed in, use it. Otherwise default
    // to 'account' (for saving core user data.)
    $category = 'account';
    if (isset($account['category'])) {
      $category = $account['category'];
      unset($account['category']);
    }

    // Drop any passed in values into the $account var. Anything
    // unused by the form just gets ignored.
    foreach ($account as $key => $value) {
      $form_state['values'][$key] = $value;
    }

    $form_state['values']['op'] = variable_get('services_user_save_button_resource_update', t('Save'));
    $form_state['values']['_category'] = $category;
    $form_state['values']['_account'] = (object)$account;
    if (isset($account['email'])) {
      $form_state['values']['email'] = $account['email'];
      $form_state['values']['confirm_email'] = $account['email'];
    }

    $ret = drupal_execute('userinfos_change_email_form', $form_state, $user, $category);
    // Error if needed.
    if ($errors = form_get_errors()) {
      return services_error(implode(" ", $errors), 406, array('form_errors' => $errors));
    }

    services_remove_user_data($account);

    return $account;
  }
  else {
      return services_error( t('Not allowed to perform this method on this type of node.'), 406);
  }
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
function _yunomi_rest_member_resource_password_forgot($case, $data) {
  // Prevent performing post action to unwanted nodes.
  global $user;
  
  if($user->uid !== 0) {
    return services_error( t('You are already logged in'), 403);
  }
  if($case !== 'password') {
    return services_error( t('Not allowed to perform this method on this type of node.'), 406);
  }
  // Adds backwards compatability with regression fixed in #1083242
  $account = _services_arg_value($data, 'data');
  // In case we haven't passed any name value.
  if (!isset($account['email']) || empty($account['email'])) {
    return services_error( t('Please, provide the email value.'), 406);
  }
  $account['name'] = $account['email'];
  // Load the required includes for saving profile information
  // with drupal_execute().
  module_load_include('inc', 'user', 'user.pages');

  // Any logged in user is by default authenticated,
  // and leaving this role set in the user's roles array
  // causes big problems because of a FAPI hack that controls
  // this checkbox on the user create and edit form (and thus
  // causes problems with drupal_execute()). Therefore we just
  // force it to 0 here.
  if (isset($account['roles'][2])) {
    $account['roles'][2] = 0;
  }

  // If a profile category was passed in, use it. Otherwise default
  // to 'account' (for saving core user data.)
  $category = 'account';
  if (isset($account['category'])) {
    $category = $account['category'];
    unset($account['category']);
  }

  // Drop any passed in values into the $account var. Anything
  // unused by the form just gets ignored.
  foreach ($account as $key => $value) {
    $form_state['values'][$key] = $value;
  }

  $form_state['values']['op'] = variable_get('services_user_save_button_resource_update', t('Save'));
  $form_state['values']['_category'] = $category;
  $form_state['values']['_account'] = (object)$account;
  if (isset($account['email'])) {
    $form_state['values']['email'] = array(
      'mail' => $account['email'],
    );
  }
  $ret = drupal_execute('user_pass', $form_state, $user, $category);
  
  // Error if needed.
  if ($errors = form_get_errors()) {
    return services_error(implode(" ", $errors), 406, array('form_errors' => $errors));
  }

  services_remove_user_data($account);

  return $account;
}
