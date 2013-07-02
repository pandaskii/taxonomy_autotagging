<?php

/**
 * @file
 * Taxonomy Auto Tagging batch helper.
 */

/**
 * Batch process all the nodes.
 */
function _taxonomy_autotagging_process_all($instance) {
  // Check if taxonomy auto tagging is active.
  $taxonomy_autotagging_active = isset($instance['settings']['taxonomy_autotagging_active']) ? $instance['settings']['taxonomy_autotagging_active'] : 0;
  if ($taxonomy_autotagging_active == 1) {
    // Get all the entity ids from the bundle.
    // Prepare the ids.
    $bundle = $instance['bundle'];
    $field_name = $instance['field_name'];
    $entity_results = db_query('select n.nid from {node} n WHERE n.type = :type AND n.nid NOT IN (select entity_id from {taxonomy_autotagging_setting} tan WHERE field_name = :field_name AND status = 0 AND entity_id = n.nid)',
      array(
        ':type' => $bundle,
        ':field_name' => $field_name,
      ));
    $entity_total = $entity_results->rowCount();
    if ($entity_total > 0) {
      $entity_ids = $entity_results->fetchCol();
      // Using Batch API for bulk processing.
      $batch = array(
        'title' => t('Taxonomy Auto Tagging'),
        'operations' => array(),
        'finished' => '_taxonomy_autotagging_batch_finished',
        'init_message' => t('Initializing...'),
        'progress_message' => t('Processing @current out of @total instances.'),
        'error_message' => t('Updating of auto tagging terms encountered an error.'),
      );

      // Prepare the taxonomy terms/synonyms.
      $synonyms_enabled = isset($instance['settings']['taxonomy_autotagging_synonyms']) ? $instance['settings']['taxonomy_autotagging_synonyms'] : 0;
      $terms = _taxonomy_autotagging_prepare_terms($field_name, $synonyms_enabled);

      foreach ($entity_ids as $id) {
        // We create a new process for each entity, as a means of keeping the.
        // batch process more transparent.
        $batch['operations'][] = array(
          '_taxonomy_autotagging_batch_process_single',
          array(
            $terms,
            $instance,
            $id,
          ),
        );
      }

      batch_set($batch);
      batch_process();
    }
    else {
      drupal_set_message(t('Taxonomy Auto Tagging cannot find the nodes which require the auto tagging, please check if you disable the auotagging with this vocabulary.'), 'warning');
      return;
    }
  }
  else {
    drupal_set_message(t('Taxonomy Auto Tagging is disabled, please enable it firstly.'), 'warning');
    return;
  }
}

/**
 * Runs the batch process for one entity.
 */
function _taxonomy_autotagging_batch_process_single($terms, $instance, $id, &$context) {
  $time_start = microtime(TRUE);
  // Get the tokenized vector at first.
  $info = entity_get_info($instance['entity_type']);
  if (!is_array($info) && empty($info)) {
    return;
  }

  $entity = $info['load hook']($id);
  $vector = _taxonomy_autotagging_entity_tokenize($entity);

  if (!empty($vector)) {
    // Update with the taxonomy auto tagging results.
    $analysis = _taxonomy_autotagging_analysis($vector, $terms, $instance);
    if (!empty($analysis) && is_array($analysis)) {
      // Get $entity language.
      $language = _taxonomy_autotagging_entity_language($entity->type, $entity, $check_language_property = TRUE);
      $entity->{$instance['field_name']}[$language] = $analysis;
      field_attach_update($instance['entity_type'], $entity);
      if ($instance['settings']['taxonomy_autotagging_storage'] == 1) {
        $taxonomy_autotagging_node = array(
          'entity_type' => $instance['entity_type'],
          'entity_id' => $id,
          'language' => $language,
          'bundle' => $instance['bundle'],
          'field_name' => $instance['field_name'],
          'data' => serialize($analysis),
          'changed' => REQUEST_TIME,
        );
        // Save them into database.
        if (is_array($taxonomy_autotagging_node) && !empty($taxonomy_autotagging_node)) {
          $existing = db_select('taxonomy_autotagging_node', 'tan')
            ->condition('entity_id', $taxonomy_autotagging_node['entity_id'])
            ->condition('field_name', $taxonomy_autotagging_node['field_name'])
            ->fields('tan', array('entity_id'))
            ->execute();
          if ($existing->fetchField() === FALSE) {
            db_insert('taxonomy_autotagging_node')->fields($taxonomy_autotagging_node)->execute();
          }
          else {
            db_update('taxonomy_autotagging_node')
              ->fields($taxonomy_autotagging_node)
              ->condition('entity_id', $taxonomy_autotagging_node['entity_id'])
              ->condition('field_name', $taxonomy_autotagging_node['field_name'])
              ->execute();
          }
        }
      }
      $msg = t('Found matching terms, and Taxonomy auto tagging now');
    }
    else {
      $msg = t('nothing found');
    }
  }
  else {
    $msg = t('the node is empty, so we ignored it to the next');
  }

  $time_end = microtime(TRUE);
  $time = $time_end - $time_start;

  // Update batch progress information.
  if (!isset($context['results']['processed'])) {
    $context['results']['processed'] = 0;
  }
  $context['results']['processed']++;
  $context['message'] = 'We are processing with node - ' . $id;
  $context['message'] .= ' - ' . $msg;
  $context['message'] .= ' - in ' . round($time, 2) . ' second(s)';
}

/**
 * Batch process finished callback.
 */
function _taxonomy_autotagging_batch_finished($success, $results, $operations) {
  if ($success) {
    $message = count($results) . ' processed by Taxonomy Auto Tagging.';
    $message .= theme('item_list', $results);
  }
  else {
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array(
      '%error_operation' => $error_operation [0],
      '@arguments' => print_r($error_operation [1], TRUE),
    ));
  }
  drupal_set_message($message);
}
