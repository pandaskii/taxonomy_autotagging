<?
/**
 * Batch process all the nodes
 * @param $instance
 */
function _taxonomy_autotagging_process_all($instance)
{
    // check if taxonomy auto tagging is active
    $taxonomy_autotagging_active = isset($instance['settings']['taxonomy_autotagging_active']) ? $instance['settings']['taxonomy_autotagging_active'] : 0;
    if ($taxonomy_autotagging_active == 1) {
        $info = entity_get_info($instance ['entity_type']);
        // get all the entity ids from the bundle
        // prepare the ids
        $bundle = $instance ['bundle'];
        $field_name = $instance ['field_name'];
        $entity_results = db_query('select n.nid from {node} n WHERE n.type = :type AND n.nid NOT IN (select entity_id from {taxonomy_autotagging_node} tan WHERE field_name = :field_name AND status = 0 AND entity_id = n.nid)',
            array(
                ':type' => $bundle,
                ':field_name' => $field_name,
            ));
        $entity_total = $entity_results->rowCount();
        if($entity_total > 0) {
            $entity_ids = $entity_results->fetchCol();
            // Using Batch API for bulk autocategorising
            $batch = array(
                'title' => t('Taxonomy Auto Tagging'),
                'operations' => array(),
                'finished' => '_taxonomy_autotagging_batch_finished',
                'init_message' => t('Initializing...'),
                'progress_message' => t('Processing @current out of @total instances.'),
                'error_message' => t('Updating of auto tagging terms encountered an error.')
            );

            //prepare the taxonomy terms/synonyms
            $synonyms_enabled = isset($instance['settings']['taxonomy_autotagging_synonyms']) ? $instance['settings']['taxonomy_autotagging_synonyms'] : 0;
            $terms = _taxonomy_autotagging_prepare_terms($field_name, $synonyms_enabled);

            foreach ($entity_ids as $id) {
                // We create a new process for each entity, as a means of keeping the
                // batch process more transparent
                $batch ['operations'] [] = array(
                    '_taxonomy_autotagging_batch_process_single',
                    array(
                        $terms,
                        $instance,
                        $id,
                        $info
                    )
                );
            }

            batch_set($batch);
            batch_process();
        } else {
            $message = t('Taxonomy Auto Tagging cannot find the nodes which require the auto tagging, please check if you disable the auotagging with this vocabulary.');
            drupal_set_message($message, 'warning');
        }
    } else {
        $message = t('Taxonomy Auto Tagging is disabled, please enable it firstly.');
        drupal_set_message($message, 'warning');
    }
}

/**
 * Runs the batch process for one entity
 */
function _taxonomy_autotagging_batch_process_single($terms, $instance, $id, $info, &$context)
{
    $time_start = microtime(true);
    //get the tokenized vector at first
    $entity = $info['load hook']($id);
    $vector = _taxonomy_autotagging_entity_tokenize($entity);

    if (!empty($vector)) {
        //update with the taxonomy auto tagging results
        $analysis = _taxonomy_autotagging_analysis($vector, $terms, $instance);
        if (!empty($analysis) && is_array($analysis)) {
            // get $entity language
            $language = _taxonomy_autotagging_entity_language($entity->type, $entity, $check_language_property = TRUE);
            $entity->{$instance['field_name']}[$language] = $analysis;
            field_attach_update($instance['entity_type'], $entity);
            $msg = t('Taxonomy auto tagging now');
        } else {
            $msg = t('nothing found');
        }
    } else {
        $msg = t('the node is empty, so we ignored it to the next');
    }

    $time_end = microtime(true);
    $time = $time_end - $time_start;

    // Update batch progress information.
    $context['results']['processed']++;
    $context['message'] = 'We are processing with node - ' . $id;
    $context['message'] .= ' - ' . $msg;
    $context['message'] .= ' - in ' . round($time, 2) . ' second(s)';
}

/**
 * Batch process finished callback.
 */
function _taxonomy_autotagging_batch_finished($success, $results, $operations)
{
    if ($success) {
        $message = count($results) . ' processed by Taxonomy Auto Tagging.';
        $message .= theme('item_list', $results);
    } else {
        $error_operation = reset($operations);
        $message = t('An error occurred while processing %error_operation with arguments: @arguments', array(
            '%error_operation' => $error_operation [0],
            '@arguments' => print_r($error_operation [1], TRUE)
        ));
    }
    drupal_set_message($message);
}
