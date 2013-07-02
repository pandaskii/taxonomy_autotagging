<?php
/**
 * @file Taxonomy Auto Tagging Text Processing helpers.
 */

/**
 * renders the entity into one string and strips the tags
 */
function _taxonomy_autotagging_entity_tokenize($entity) {
  $vector = '';
  $language = _taxonomy_autotagging_entity_language($entity->type, $entity, $check_language_property = TRUE);
  // run drupal function to get the plain text
  // prepare the text for matching
  $string = $entity->title;
  $string .= $entity->body[$language][0]['safe_value'];
  if (!empty($string)) {
    $string = strip_tags(drupal_html_to_text($string));
    $string = preg_split('/((^\p{P}+)|(\p{P}*\s+\p{P}*)|(\p{P}+$))/', $string, -1, PREG_SPLIT_NO_EMPTY);
    $vector = implode(' ', $string);
  }
  return $vector;
}

/**
 * @param string $vector
 * @param array $terms
 * @param object $instance
 * @return array
 */
function _taxonomy_autotagging_analysis($vector, $terms, $instance) {
  if ($instance['settings']['taxonomy_autotagging_active'] == 0) {
    return;
  }
  // if case sensitive, then ignore the following lower case process
  if ($instance['settings']['taxonomy_autotagging_case'] != 1) {
    $vector = strtolower($vector);
  }
  // get the max limit from the field settings
  $field = field_info_field($instance['field_name']);
  $max = $field['cardinality'] = -1 ? 100 : $field['cardinality'];
  // start the matching analysis process
  $fl_array = array();
  foreach ($terms as $term) {
    // stop matching analysis process while over the max tags settings
    if (count($fl_array) >= $max) {
      drupal_set_message(t('stopping the process because the max number has been achieved with the max values setting - :max'), array(':max' => $max));
      break;
    }
    foreach ($term['name'] as $key) {
      $flag = FALSE;
      if ($instance['settings']['taxonomy_autotagging_case'] != 1) {
        $key = strtolower($key);
      }
      // analysis the target text with methods regarding the different mode
      switch ($instance['settings']['taxonomy_autotagging_mode']) {
        case 0:
          if (is_int(strpos($vector, $key))) {
            $fl_array[] = array(
              'tid' => $term['tid'],
              'mapping' => '',
            );
            $flag = TRUE;
          }
          break;
        case 1:
          if (is_int(strpos($vector, $key))) {
            $fl_array[] = array(
              'tid' => $term['tid'],
              'mapping' => '',
            );
            $flag = TRUE;
          }
          elseif (is_int(strpos($vector, text_processor_en::pluralize($key)))) {
            $fl_array[] = array(
              'tid' => $term['tid'],
              'mapping' => '',
            );
            $flag = TRUE;
          }
          else {
            if (is_int(strpos($vector, text_processor_en::singularize($key)))) {
              $fl_array[] = array(
                'tid' => $term['tid'],
                'mapping' => '',
              );
              $flag = TRUE;
            }
          }
          break;
        case 2:
          if (preg_match('/' . $key . '/', $vector, $matches)) {
            $fl_array[] = array(
              'tid' => $term['tid'],
              'mapping' => $matches[0],
            );
            $flag = TRUE;
          }
          break;
        case 3:
          if (preg_match("/\b(" . $key . "[ieds]*)\b/i", $vector, $matches)) {
            $fl_array[] = array(
              'tid' => $term['tid'],
              'mapping' => $matches[0],
            );
            $flag = TRUE;
          }
          break;
        case 4:
          if (preg_match('/' . $key . '/', $vector, $matches)) {
            $fl_array[] = array(
              'tid' => $term['tid'],
              'mapping' => $matches[0],
            );
            $flag = TRUE;
          }
          elseif (preg_match('/' . text_processor_en::pluralize($key) . '/', $vector, $matches)) {
            $fl_array[] = array(
              'tid' => $term['tid'],
              'mapping' => $matches[0],
            );
            $flag = TRUE;
          }
          else {
            if (preg_match('/' . text_processor_en::singularize($key) . '/', $vector, $matches)) {
              $fl_array[] = array(
                'tid' => $term['tid'],
                'mapping' => $matches[0],
              );
              $flag = TRUE;
            }
          }
          break;
      }
      // if found matching, then break the loop to save performance
      if ($flag === TRUE) {
        break 1;
      }
    }
  }
  // if the taxonomy filed is required and we can find nothing, we will use the last one to avoid the error.
  if ($instance['required'] && empty($fl_array)) {
    $fl_array = array_pop(array_keys($terms));
  }
  return $fl_array;
}

/**
 * return the terms array/synonyms if the synonyms module is enabled.
 */
function _taxonomy_autotagging_prepare_terms($field_name, $synonyms_enabled) {
  $terms_array = array();
  $field = field_info_field($field_name);
  $vocabulary = taxonomy_vocabulary_machine_name_load($field["settings"]["allowed_values"][0]["vocabulary"]);
  $tree = taxonomy_get_tree($vocabulary->vid, 0, NULL, TRUE);
  foreach ($tree as $term) {
    $terms = array($term->name);
    if ($synonyms_enabled == 1) {
      $synonyms = synonyms_get_term_synonyms($term);
      foreach ($synonyms as $synonym) {
        $terms[] = $synonym['value'];
      }
    }
    $terms_array[] = array(
      'tid' => $term->tid,
      'name' => $terms,
    );
  }
  return $terms_array;
}

/**
 * Returns the language code of the given entity.
 */
function _taxonomy_autotagging_entity_language($entity_type, $entity, $check_language_property = TRUE) {
  $langcode = NULL;

  if (function_exists('entity_language')) {
    $langcode = entity_language($entity_type, $entity);
  }
  elseif ($check_language_property && !empty($entity->language)) {
    $langcode = $entity->language;
  }

  return !empty($langcode) ? $langcode : LANGUAGE_NONE;
}