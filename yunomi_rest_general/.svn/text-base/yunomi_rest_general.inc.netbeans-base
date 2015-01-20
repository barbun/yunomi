<?php 
/**
* @file
* Helper functions set.
*/

/**
 * Callback for retrieving views resources.
 *
 * @param $view_name
 *   String. The views name.
 * @param $display_id
 *   String (optional). The views display name.
 * @param $offset
 *   Integer (optional). An offset integer for paging.
 * @param $limit
 *   Integer (optional). A limit integer for paging.
 * @param $args
 *   Array (optional). A list of arguments to pass to the view.
 * @param $return_type
 *   String (optional). Whether to return the raw data results (FALSE), the entire views object ('view') or themed results ('theme').
 * @param $args
 *   Array (optional). A list of exposed filters to pass to the view.
 *
 * @return
 *  Array. The views return.
 */
function _yunomi_rest_views_retrieve($view_name, $display_id = 'default', $args = array(), $offset = 0, $limit = 10, $return_type = FALSE, $filters = array()) {
    
  $result = array();
  $view = views_get_view($view_name);
  
  if (!$view) {
    watchdog('yunomi_rest_general', '@function @name - view could not be found', array('@function' => '_yunomi_rest_views_retrieve', '@name' => $view_name), WATCHDOG_ERROR);
    return services_error( t('Internal Server Error.'), 500);
  }
  // Disable caching to get offset and limit working.
  $view->display[$display_id]->display_options['cache'] = array('type' => 'none');
  // Put all arguments eand filters and then execute.
  $view->set_arguments($args, FALSE);
  $view->set_exposed_input($filters);
  $view->set_offset($offset);
  
  // Support for Views 2
  if (method_exists($view, 'set_use_pager')) {
    // If offset is set we can't have a user pager.
    if (empty($offset)) {
      $view->set_use_pager(TRUE);
      $view->set_items_per_page($limit);
    }
    else {
      // Disable the user pager.
      $view->set_use_pager(FALSE);
      $view->set_items_per_page($limit);
    }
  }
  else {
    $view->set_items_per_page($limit);
  }
  if (empty($return_type)) {
    $view->set_display($display_id);
    $view->execute();
    $result = $view->result;

    $row_plugin = $view->display_handler->get_option('row_plugin');
    // If row plugin is node, then we should load each node
    if ($row_plugin == 'node') {
      $nodes = array();
      foreach ($view->result as $row) {
        $nodes[] = services_node_load(node_load($row->nid));
      }
      $result = $nodes;
    }
    elseif ($row_plugin == 'fields') {
      $items = array();
      foreach ($view->result as $row) {
        $item = new stdClass();
        foreach ($view->field as $field_id => $field) {
          if (!empty($field->options['label'])) {
            $field_label = strtolower($field->options['label']);
          }
          elseif ($field->field_alias) {
            $field_label = $field->field_alias;
          }
          else {
            $field_label = $field_id;
          }
          $field->pre_render(array($row));
          $item->$field_label = $field->render($row);
        }
        $items[] = $item;
      }
      if ($items) {
        $result = $items;
      }
    }
  }
  elseif ($return_type == 'view') {
    $view->set_display($display_id);
    $view->execute();
    return $view;
  }
  else {
    // We want to keep the result an array.
    $result[] = $view->preview($display_id);
  }
  return $result;
}

///// THEMING and FORMATTING functions. //////

/**
 * Callback to format the node fields and return in as result array.
 * 
 * @param $node
 * Object. The object of the node that needs to be formatted.
 * 
 * @return
 *  Array. Result fields of the formated node.
 */
function _yunomi_rest_format_article($node) {

  // Copy needed fields to a new array.
  $tree = badges_get_badge_tree_flat();
  
  $result = $badges = $hubs= array();
  
  foreach($node->taxonomy as $tid => $term) {
    if(isset($tree[$tid]) && $tree[$tid]->type == 'hub') {
      $hubs[] = $tree[$tid];
    }
    if(isset($tree[$tid]) && ( $tree[$tid]->type == 'badge' ) ) {
      $badges[] = $tree[$tid];
    }
  }
  $node->badges = $badges;
  
  $node->thema = NULL;
  if(isset($hubs[0])) {$node->thema = $hubs[0] ;}
  
  // Now organize the new object.
  $result['nid'] = $node->nid;
  $result['title'] = $node->title;
  $result['image'] = $node->field_article_image[0]['filepath'];
  
  $result['intro'] = preg_replace('/(<[^>]+) style=".*?"/i', '$1', $node->field_article_intro[0]['value']);
  $result['comment_count'] = $node->comment_count;
  
  $result['author']['uid'] = $node->uid;
  $result['author']['name'] = $node->name;
  $result['author']['picture'] = $node->picture;
  
  $result['taxonomy'] = array_values($node->taxonomy);
  $result['badges'] = $node->badges;
  $result['thema'] = $node->thema;
  $result['body_formatted'] = '<div class="intro">' . $node->field_article_intro[0]['value'] . '</div>';
  
  if (is_array($node->field_article_paragraph_text)) {
    foreach ($node->field_article_paragraph_text as $key => $paragraph) {
      $img = NULL;
      if (!empty($node->field_article_paragraph_image[$key]['filepath'])) {
        $img = $node->field_article_paragraph_image[$key]['filepath'];
      }
      $result['body'][$key]['title'] = check_plain($node->field_article_paragraph_title[$key]['value']);
      if (!empty($img)) {
        $result['body'][$key]['image'] = $img; 
      }
      $result['body'][$key]['text'] = preg_replace('/(<[^>]+) style=".*?"/i', '$1',$node->field_article_paragraph_text[$key]['value']);
      
      // Send the current body to the format function and get a result.
      $result['body_formatted'] .= theme_yunomi_format_fields($result['body'][$key], $key);
    }
  }
  // Remove style attribute from body to get rid of styling problems.
  $result['body_formatted'] = preg_replace('/(<[^>]+) style=".*?"/i', '$1', $result['body_formatted']);
  $result['created'] = $node->created;
  if (isset($node->yunomitv_mediaclipid) && !empty($node->yunomitv_mediaclipid)) {
    $result['bbwid'] = $node->yunomitv_mediaclipid;
  }
  return $result;
}

/**
 * Callback to format the node fields of the  and return in as result array.
 * 
 * @param $node
 * Object. The object of the node that needs to be formatted.
 * 
 * @return
 *  Array. Result fields of the formated node.
 */
function _yunomi_rest_format_article_queue($node) {
  // Copy needed fields to a new array.
  $result = array();
  
  $node->badges = badges_get_node_badges($node->nid);
  $node->thema = _yunomi_rest_article_get_thema_hubs($node->taxonomy);
  // Now organize the new object.
  $result['nid'] = $node->nid;
  $result['title'] = $node->title;
  $result['image'] = $node->field_article_image[0]['filepath'];
  if (!empty($result['image'])) {
    $result['image_2'] = imagecache_create_url('to be defined', $result['image'], FALSE);
  }
  
  $result['intro'] = preg_replace('/(<[^>]+) style=".*?"/i', '$1', $node->field_article_intro[0]['value']);
  $result['comment_count'] = $node->comment_count;
  
  $result['author']['uid'] = $node->uid;
  $result['author']['name'] = $node->name;
  $result['author']['picture'] = $node->picture;
  
  $result['taxonomy'] = array_values($node->taxonomy);
  $result['badges'] = $node->badges;
  $result['thema'] = $node->thema;
  $result['body_formatted'] = '<div class="intro">' . $node->field_article_intro[0]['value'] . '</div>';
  
  if (is_array($node->field_article_paragraph_text)) {
    foreach ($node->field_article_paragraph_text as $key => $paragraph) {
      $img = NULL;
      if (!empty($node->field_article_paragraph_image[$key]['filepath'])) {
        $img = $node->field_article_paragraph_image[$key]['filepath'];
      }
      $result['body'][$key]['title'] = check_plain($node->field_article_paragraph_title[$key]['value']);
      if (!empty($img)) {
        $result['body'][$key]['image'] = $img; 
      }
      $result['body'][$key]['text'] = preg_replace('/(<[^>]+) style=".*?"/i', '$1', $node->field_article_paragraph_text[$key]['value']);
      
      // Send the current body to the format function and get a result.
      $result['body_formatted'] .= theme_yunomi_format_fields($result['body'][$key], $key);
    }
  }
  // Remove style attribute from body to get rid of styling problems.
  $result['body_formatted'] = preg_replace('/(<[^>]+) style=".*?"/i', '$1', $result['body_formatted']);
  $result['created'] = $node->created;
  if (isset($node->yunomitv_mediaclipid) && !empty($node->yunomitv_mediaclipid)) {
    $result['yunomitv_mediaclipid'] = $node->yunomitv_mediaclipid;
  }
  
  return $result;
}


/**
 * Callback to format the node fields and return in as result array.
 * 
 * @param $item
 * Array. One of the result of view query.
 * 
 * @return
 *  Array. Result fields of the formated node.node
 */
function _yunomi_rest_format_recipe($item) {
  // Copy needed fields to a new array.
  $result = array();
  
  // Now organize the new object.
  $result['nid'] = $item->nid;
  $result['title'] = $item->node_title;
  if (isset($item->rating) && !empty($item->rating)) {
    $result['rating'] = $item->rating;
  }
  
  $result['image'] = $item->node_data_field_recipe_img_field_recipe_img_fid;
  $result['image_livelink'] = $item->livelink_image;
  
  $result['display_date'] = $item->queue_items_displaydate;
  
  return $result;
}

/**
 * Callback for organizing output array of testtells and recipes of the week.
 * 
 * @param $node
 * Object. The object of the node that needs to be formatted.
 * 
 * @return
 *  Array. Result fields of the formated node.
 */
function _yunomi_rest_format_testtells($node) {
  // Copy needed fields to a new array.
  $result = array();
  
  // Now organize the new object.
  $result['nid'] = $node->nid;
  $result['title'] = $node->title;
  
  if ($node->type == "recipe") {
    $result['image'] = $node->field_recipe_photo[0]['filepath'];
  }
  else {
    $result['image'] = $node->field_test_tell_img[0]['filepath'];
    $result['intro'] = $node->field_test_tell_intro[0]['value'];
  }
  
  if (isset($node->rating) && !empty($node->rating)) {
    $result['rating'] = $node->rating;
  }
  
  $result['comment_count'] = $node->comment_count;
  $result['body_formatted'] = '<div class="intro">' . $node->field_test_tell_intro[0]['value'] . '</div>';
  
  if (is_array($node->field_test_tell_paragraph_text)) {
    foreach ($node->field_test_tell_paragraph_text as $key => $paragraph) {
      $img = NULL;
      if (!empty($node->field_test_tell_paragraph_img[$key]['filepath'])) {
        $img = $node->field_test_tell_paragraph_img[$key]['filepath'];
      }
      
      $result['body'][$key]['title'] = check_plain($node->field_test_tell_paragraph_subt[$key]['value']);
      if (!empty($img)) {
        $result['body'][$key]['image'] = $img; 
      }
      $result['body'][$key]['text'] = $node->field_test_tell_paragraph_text[$key]['value'];
      
      // Send the current body to the format function and get a result.
      $result['body_formatted'] .= theme_yunomi_format_fields($result['body'][$key], $key);
    }
  }
  
  $result['taxonomy'] = $node->taxonomy;
  $result['created'] = $node->created;
  
  if (isset($node->questions) && !empty($node->questions)) {
    $temp = $node->questions['openquestion']['#value'];
    unset($node->questions['openquestion']['#value']);
    
    $result['questions'] = $node->questions;
    $result['questions']['openquestion']['title'] = $temp;
  }
  
  return $result;
}

/**
 * Callback for organizing output array of testtells and recipes of the week.
 * 
 * @param $node
 * Object. The object of the node that needs to be formatted.
 * 
 * @return
 *  Array. Result fields of the formated node.
 */
function _yunomi_rest_format_recipes($recipe) {
  // Copy needed fields to a new array.
  $result = array();
  
  // Get all necessary fields.
  $results['nid'] = $recipe->nid;
  $results['livelinkid'] = $recipe->dataid;
  $results['title'] = $recipe->recipe_fields['titel'];
  // Check if we have big image, if not then use small one.
  if(!empty($recipe->recipe_fields['media']['recept_image_groot'])) {
    $results['image'] = $recipe->recipe_fields['media']['recept_image_groot']['url'];
  }
  else {
    $results['image'] = $recipe->recipe_fields['media']['recept_image']['url'];
  }
  
  $results['ratings'] = $recipe->recipe_fields['rating'];
  $results['comment_count'] = $recipe->comment_count;
  $results['body'] = theme_yunomi_ll_recipe($recipe->recipe_fields);
  $results['brands'] = theme_yunomi_ll_recipe_brands($recipe->recipe_fields);
  // Provide all unformatted fields just for the case we need something from there.
  $results['fields'] = $recipe->recipe_fields;
  
  return $results;
  
  return $result;
}


/**
 * Helper function for theming the node fields.
 * 
 * @param $body
 * Object. An object that needs to be formatted.
 * 
 * @param $key
 * Int. The key representing which of the paragraphs where used.
 * 
 * @return
 *  Array.
 */
function theme_yunomi_format_fields($body, $key) {
  $output = '<div class=paragraph_' . $key . '>';
  foreach ($body as $key => $value) {
    if ($key == 'image') {
      $output .= '<div class="' . $key . '"><img src="' . $value . '" /></div>';
    }
    else {
      $output .= '<div class="' . $key . '">' . $value . '</div>';
    }
  }
  $output .= '</div>';
  
  return $output;
}


/**
 * Callback for retrieving views resources.
 *
 * @param $view_name
 *   String. The views name.
 * @param $display_id
 *   String (optional). The views display name.
 * @param $offset
 *   Integer (optional). An offset integer for paging.
 * @param $limit
 *   Integer (optional). A limit integer for paging.
 * @param $args
 *   Array (optional). A list of arguments to pass to the view.
 * @param $return_type
 *   String (optional). Whether to return the raw data results (FALSE), the entire views object ('view') or themed results ('theme').
 * @param $args
 *   Array (optional). A list of exposed filters to pass to the view.
 *
 * @return
 *  Array. The views return.
 */
function _yunomi_rest_views_retrieve_ignore_tid($view_name, $display_id = 'default', $args = array(), $offset = 0, $limit = 10, $return_type = FALSE, $filters = array()) {
  $result = array();
  $view = views_get_view($view_name);
  
  if (!$view) {
    watchdog('yunomi_rest_general', '@function @name - view could not be found', array('@function' => '_yunomi_rest_views_retrieve', '@name' => $view_name), WATCHDOG_ERROR);
    return services_error( t('Internal Server Error.'), 500);
  }
  // Disable caching to get offset and limit working.
  $view->display[$display_id]->display_options['cache'] = array('type' => 'none');
  // Put all arguments eand filters and then execute.
  $view->set_arguments($args, FALSE);
  $view->set_exposed_input($filters);
  $view->set_offset($offset);
  // Ignore argument absence.
  $view->display[$display_id]->display_options['arguments']['tid']['default_action'] = "ignore"; 
  // Set default params as well.
  $view->display['default']->display_options['cache'] = array('type' => 'none');
  $view->display['default']->display_options['arguments']['tid']['default_action'] = "ignore"; 
  // Support for Views 2
  if (method_exists($view, 'set_use_pager')) {
    // If offset is set we can't have a user pager.
    if (empty($offset)) {
      $view->set_use_pager(TRUE);
      $view->set_items_per_page($limit);
    }
    else {
      // Disable the user pager.
      $view->set_use_pager(FALSE);
      $view->set_items_per_page($limit);
    }
  }
  else {
    $view->set_items_per_page($limit);
  }
  if (empty($return_type)) {
    $view->set_display($display_id);
    $view->execute();
    $result = $view->result;

    $row_plugin = $view->display_handler->get_option('row_plugin');
    // If row plugin is node, then we should load each node
    if ($row_plugin == 'node') {
      $nodes = array();
      foreach ($view->result as $row) {
        $nodes[] = services_node_load(node_load($row->nid));
      }
      $result = $nodes;
    }
    elseif ($row_plugin == 'fields') {
      $items = array();
      foreach ($view->result as $row) {
        $item = new stdClass();
        foreach ($view->field as $field_id => $field) {
          if (!empty($field->options['label'])) {
            $field_label = strtolower($field->options['label']);
          }
          elseif ($field->field_alias) {
            $field_label = $field->field_alias;
          }
          else {
            $field_label = $field_id;
          }
          $field->pre_render(array($row));
          $item->$field_label = $field->render($row);
        }
        $items[] = $item;
      }
      if ($items) {
        $result = $items;
      }
    }
  }
  elseif ($return_type == 'view') {
    $view->set_display($display_id);
    // If we overriden the type of the node that's gonna be displayed
    if (isset($filters['type']) && ($filters['type'] == 'recipe')) {
      // Perform pre execute and alter query to set it to desired content type.
      $view->preview();
      $view->build_info['query_args'][0] = "recipe";
    } 
    $view->execute();
    return $view;
  }
  else {
    // We want to keep the result an array.
    $result[] = $view->preview($display_id);
  }
  return $result;
}

/*
 * Helper theme function for recipe fields as html.
 * 
 * param $fields
 * Array of recipe fields.
 * 
 * return
 * Array with brands or an empty array.
 * 
 */
function theme_yunomi_ll_recipe($fields, $path_to_theme = NULL) {
  global $language;
  global $user;
  $country = helper_get_site_country();

  $output = '';
  $output .= '<div>';
  $output .= '<h1>' . $fields['titel'] . '</h1>';
  
  $output .= '<h2>' . t('Benodigdheden') . '</h2>';
  $output .= '<ul class="ingredients">';
  if(!empty($fields['ingredienten'])) {
    foreach($fields['ingredienten'] as $ingredient){
      $output .= '<li>';
      $output .= '<p class="grey_text">';
      $output .= trim($ingredient['Aantal']['value']) != '' ? $ingredient['Aantal']['value'] . ' ' . $ingredient['Eenheid']['value'] . ' ' : ' ' ;
      if (!empty($ingredient['Product link']['value']) && is_numeric($ingredient['Product link']['value'])) {
        $output .= l($ingredient['Ingredient']['value'], 'recipe/product/' . $ingredient['Product link']['value'], array('attributes' => array('id' => 'recipe_product_popup', 'class' => 'popups')));
      } else {
        $output .= $ingredient['Ingredient']['value'];
      }
      $output .= '</p>';
      $output .= '</li>';
    }
  }
  $output .= '</ul>';

  if (!empty($fields['ster-ingredient'])){
    $output .= '<h2>' . t('Ster-ingrediënt') . '</h2>';
    $output .= preg_replace('/\\n/', '<br />', $fields['ster-ingredient']);
  }

  if (!empty($fields['bereidingstechniek'])){
    $output .= '<h2>' . t('Bereidings techniek') . '</h2>';
    $output .= '<span>' . preg_replace('/\\n/', '<br />', $fields['bereidingstechniek']).'</span>';
  }
  
  if (!empty($fields['voorbereiding'])){
    $output .= '<h2>' . t('Voorbereiding') . '</h2>';
    $output .= preg_replace('/\\n/', '<br />', $fields['voorbereiding']);
  }
  
  if (!empty($fields['bereiding'])){
    $output .= '<h2>' . t('Bereiding') . '</h2>';
    $output .= preg_replace('/\\n/', '<br />', $fields['bereiding']);
  }
  
  if (!empty($fields['serveer_tips'])){
    $output .= '<h2>' . t('Serveer tips') . '</h2>';
    $output .= preg_replace('/\\n/', '<br />', $fields['serveer_tips']);
  }
  if (!empty($fields['variaties'])){
    $output .= '<h2>' . t('Variaties') . '</h2>';
    $output .= preg_replace('/\\n/', '<br />', $fields['variaties']);
  }
  $output .= '</span>';
  
  $output .= '<meta content="' . $fields['voorbereiding'] . ' ' . $fields['bereiding'] . ' ' .  $fields['serveer_tips'] . ' ' . $fields['variaties'] . '">';
  
  $rendered_nutri = '';
  if(!empty($fields['voedingswaarde'])) {    
    foreach($fields['voedingswaarde'] as $item){
      if(!empty($item['value'])) {
        $rendered_nutri .= '<li><span class="bold voedingswaarde">' . t($item['name']) . ': </span> ' . $item['value'] .  $item['quantifier'] . '</li>';
      }
    }
  }
  
  if (!empty($rendered_nutri)) {
    $output .= '<h2>' . t('Voedingswaarde') . '</h2>';
    $output .= '<ul>';
    $output .= $rendered_nutri;
    $output .= '</ul>';
  }

  $output .= '<div class="divider"> </div><div> </div>';
  
  $output .= '</div>';
  
  return $output;
}


/*
 * Helper theme function for recipe brands.
 * 
 * param $fields
 * Array of recipe fields.
 * 
 * return
 * Array with brands or an empty array.
 * 
 */
function theme_yunomi_ll_recipe_brands($fields) {
  global $language;
  global $user;
  $country = helper_get_site_country();
  // The brand banner
  $brand_data = ll_recipe_get_brand_data();
  $code = $country . '_' . $language->language;
  if (!empty($fields['merknaam']) && !empty($brand_data[$code][$fields['merknaam']])){
  	return array('name' => $brand_data[$code][$fields['merknaam']][0], 'logo' => theme('imagecache' , 'll_recipe_banner_brand_img_content_footer_w84' , drupal_get_path('module','ll_recipe') . '/brands/' . $brand_data[$code][$fields['merknaam']][0] . '.png'), 'url' => $brand_data[$code][$fields['merknaam']][1]);
  }
  return array();
}

function theme_yunomi_ll_recipe_product($ws_data, $img = NULL) {

  $output .= '<h1>' . $ws_data['Product_CW']['content']['Titel']['value'] . '</h1>';

  if(trim($ws_data['Product_CW']['content']['Productomschrijving']['data']) != ''){
    $output .= '<p>' . $ws_data['Product_CW']['content']['Productomschrijving']['data'] . '</p>';
    $style = '';
  } else {
    $style = ' style="margin-top:0;"';
  }

  // Inhoud
  if (!empty($ws_data['Product_CW']['content']['Inhoud']['data'])){
    $output .= '<p><br /><strong>' . t('Inhoud') . ': </strong>' . $ws_data['Product_CW']['content']['Inhoud']['data'] . '</p>';
  }

  // Gewicht en e-teken
  if (!empty($ws_data['Product_CW']['content']['Gewicht en e-teken']['data'])){
    $output .= '<p><br /><strong>' . t('Gewicht en e-teken') . ': </strong>' . $ws_data['Product_CW']['content']['Gewicht en e-teken']['data'] . '</p>';
  }

  // Vegetarisch
  if (!empty($ws_data['Product_CW']['content']['Vegetarisch']['value'])){
    $output .= '<p><br /><strong>' . t('Vegetarisch') . ': </strong>' . $ws_data['Product_CW']['content']['Vegetarisch']['value'] . '</p>';
  }

  $output .= '<br /><h2'.$style.'>' . t('Nutritional information') . '</h2>';
  $nutri = $ws_data['Product_CW']['content']['Voedingswaardentabel big 4']['data'];
  $nutri = preg_replace('/\\n/', '<br />', $nutri);
  $output .= '<p>' . $nutri . '</p>';
  //$output .= '<p>' . $ws_data['content']['Gewicht en e-teken']['data'] . '</p><br />';
  if (!empty($ws_data['Product_CW']['content']['Ingredienten declaratie incl. KWID']['data'])){
    $output .= '<h2>' . t('Ingredients') . '</h2>';
    $output .= '<p>' . $ws_data['Product_CW']['content']['Ingredienten declaratie incl. KWID']['data'] . '</p>';
  }

  // Allergenen info.

  // Collect info first, might not contain any allergen information.
  $items = array();
  $allergenen = array('Granen met gluten', 'Schaal- en schelpdieren', 'Kippenei', 'Vis', 'Pinda', 'Soja', 'Melk', 'Noten', 'Sesamzaad', 'Mosterd', 'Selderij', 'Erwten', 'Sulfiet', 'Lupine', 'Weekdieren');
  foreach ($allergenen as $label) {
    if (!empty($ws_data['Xtra_Albalijst']['content'][$label]['value'])) {
      $items[] = t($label) . ': ' . t($ws_data['Xtra_Albalijst']['content'][$label]['value']);
    }
  }
  if (!empty($items)) {
    $output .= '<h2>' . t('Allergenen') . '</h2>';
    $output .= theme('item_list', $items);
  }

  $website_txt = $ws_data['Product_CW']['sets']['Website'][0]['Label']['value'];
  $website_link = $ws_data['Product_CW']['sets']['Website'][0]['Url']['value'];

  if($website_txt && $website_link) {
    $output .= '<p>&nbsp;</p>';
    $target = '_blank';
    $output .= theme('link_button', $website_txt, $website_link, NULL, NULL, NULL, $target);
  }

  return $output;
}


/*
 * Helper function to get thema hubs for article node.
 * 
 * param $taxonomy
 * Array of taxonomy for certain node.
 * 
 * return
 * Array of thema taxonomy.
 * 
 */
function _yunomi_rest_article_get_thema_hubs($taxonomy) {
  $hubs = array();
  // go throught array and cherry pick needed taxonomy.
  foreach ($taxonomy as $key => $value) {
    if ($value->vid == 9) {
      foreach ($value as $param => $def) {
        if ($value !== 'tid') {
          $hubs[$param] = $def;   
        }
      }
    }
  }
  
  return $hubs;
}


/**
 * Callback for retrieving views resources.
 *
 * @param $view_name
 *   String. The views name.
 * @param $display_id
 *   String (optional). The views display name.
 * @param $offset
 *   Integer (optional). An offset integer for paging.
 * @param $limit
 *   Integer (optional). A limit integer for paging.
 * @param $args
 *   Array (optional). A list of arguments to pass to the view.
 * @param $return_type
 *   String (optional). Whether to return the raw data results (FALSE), the entire views object ('view') or themed results ('theme').
 * @param $args
 *   Array (optional). A list of exposed filters to pass to the view.
 *
 * @return
 *  Array. The views return.
 */
function _yunomi_rest_views_retrieve_add_term($view_name, $display_id = 'default', $args = array(), $offset = 0, $limit = 10, $return_type = FALSE, $filters = array()) {
  $result = array();
  $view = views_get_view($view_name);
  
  if (!$view) {
    watchdog('yunomi_rest_general', '@function @name - view could not be found', array('@function' => '_yunomi_rest_views_retrieve', '@name' => $view_name), WATCHDOG_ERROR);
    return services_error( t('Internal Server Error.'), 500);
  }
  // Disable caching to get offset and limit working.
  $view->display[$display_id]->display_options['cache'] = array('type' => 'none');
  // Put all arguments eand filters and then execute.
  $view->set_arguments($args, FALSE);
  $view->set_exposed_input($filters);
  $view->set_offset($offset);
  // Ignore argument absence.
  $view->display[$display_id]->display_options['arguments']['tid']['default_action'] = "ignore"; 
  // Set default params as well.
  $view->display['default']->display_options['cache'] = array('type' => 'none');
  $view->display['default']->display_options['arguments']['tid']['default_action'] = "ignore"; 
  // Support for Views 2
  if (method_exists($view, 'set_use_pager')) {
    // If offset is set we can't have a user pager.
    if (empty($offset)) {
      $view->set_use_pager(TRUE);
      $view->set_items_per_page($limit);
    }
    else {
      // Disable the user pager.
      $view->set_use_pager(FALSE);
      $view->set_items_per_page($limit);
    }
  }
  else {
    $view->set_items_per_page($limit);
  }
  if (empty($return_type)) {
    $view->set_display($display_id);
    $view->execute();
    $result = $view->result;

    $row_plugin = $view->display_handler->get_option('row_plugin');
    // If row plugin is node, then we should load each node
    if ($row_plugin == 'node') {
      $nodes = array();
      foreach ($view->result as $row) {
        $nodes[] = services_node_load(node_load($row->nid));
      }
      $result = $nodes;
    }
    elseif ($row_plugin == 'fields') {
      $items = array();
      foreach ($view->result as $row) {
        $item = new stdClass();
        foreach ($view->field as $field_id => $field) {
          if (!empty($field->options['label'])) {
            $field_label = strtolower($field->options['label']);
          }
          elseif ($field->field_alias) {
            $field_label = $field->field_alias;
          }
          else {
            $field_label = $field_id;
          }
          $field->pre_render(array($row));
          $item->$field_label = $field->render($row);
        }
        $items[] = $item;
      }
      if ($items) {
        $result = $items;
      }
    }
  }
  elseif ($return_type == 'view') {
    $view->set_display($display_id);
    // If we overriden the type of the node that's gonna be displayed
    if (isset($filters['type']) && ($filters['type'] == 'recipe')) {
      // Perform pre execute and alter query to set it to desired content type.
      $view->preview();
      $view->build_info['query_args'][0] = "recipe";
    } 
    $view->execute();
    return $view;
  }
  else {
    // We want to keep the result an array.
    $result[] = $view->preview($display_id);
  }
  return $result;
}

/*
 * Helper function to check special characters used for terms.
 */
function _yunomi_rest_brands_checkterm($name) {
  $symbols = array(
    '+',
    '.',
    'È',
    'É',
    'À',
    'Â',
    'Ã',
    'Ä',
    'Ç',
    'È',  
    'É',
    'Ê',
    'Ë',
    'Î',
    'Ï',
    'Ô',
    'Ù',  
    'Û',
    'Ü',
    'Ý',
    'ý',
    'ü',
    'û',
    'ù',
    'ô',
    'ï',
    'î',
    'ë',
    'ê',
    'é',
    'è',
    'ç',
    'à',
    'á',
    'â',
    'ä ',
  );
  $name_arr = str_split_php4_utf8($name);
  foreach ($name_arr as $key => $value) {
    if (in_array($value, $symbols)) {
      $value2 = drupal_urlencode($value);
      $name = str_replace($value, $value2, $name);
    }
  }  
  
  return $name;
}


/*
 * Helper function to place each character of the string into and array.
 */
function str_split_php4_utf8($str) { 
    $split=1; 
    $array = array(); 
    for ( $i=0; $i < strlen( $str ); ){ 
        $value = ord($str[$i]); 
        if($value > 127){ 
            if($value >= 192 && $value <= 223) 
                $split=2; 
            elseif($value >= 224 && $value <= 239) 
                $split=3; 
            elseif($value >= 240 && $value <= 247) 
                $split=4; 
        }else{ 
            $split=1; 
        } 
            $key = NULL; 
        for ( $j = 0; $j < $split; $j++, $i++ ) { 
            $key .= $str[$i]; 
        } 
        array_push( $array, $key ); 
    } 
    return $array; 
} 