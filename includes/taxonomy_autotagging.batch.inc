<?
/**
 * Batch process all the nodes
 * @param $instance
 */
function _taxonomy_autotagging_process_all($instance) {
    // check if taxonomy auto tagging is active
    $taxonomy_autotagging_active = isset($instance['settings']['taxonomy_autotagging_active']) ? $instance['settings']['taxonomy_autotagging_active'] : 0;
    if ($taxonomy_autotagging_active == 1) {
        $info = entity_get_info ( $instance ['entity_type'] );
        // get all the entity ids from the bundle
        // prepare the ids
        $entity_type = $instance ['entity_type'];
        $bundle = $instance ['bundle'];
        $field_name = $instance ['field_name'];
        $entity_ids = db_select('taxonomy_autotagging_node','tan')
            ->condition('entity_type',$entity_type)
            ->condition('bundle',$bundle)
            ->condition('field_name',$field_name)
            ->condition('status',1)
            ->fields('tan',array('entity_id'))
            ->execute()
            ->fetchCol();
        // Using Batch API for bulk autocategorising
        $batch = array (
            'title' => t ( 'Taxonomy Auto Tagging' ),
            'operations' => array (),
            'finished' => '_taxonomy_autotagging_batch_finished',
            'init_message' => t ( 'Initializing...' ),
            'progress_message' => t ( 'Processing @current out of @total instances.' ),
            'error_message' => t ( 'Updating of auto tagging terms encountered an error.' )
        );

        //preoare the taxonomy terms/synonyms
        $terms = _taxonomy_autotagging_prepare_terms($field_name);

        foreach ( $entity_ids as $id ) {
            // We create a new process for each entity, as a means of keeping the
            // batch process more transparent
            $batch ['operations'] [] = array (
                '_taxonomy_autotagging_batch_process_single',
                array (
                    $terms,
                    $instance,
                    $id,
                    $info
                )
            );
        }

        batch_set ( $batch );
        batch_process ();
    } else {
        $message = t('Taxonomy Auto Tagging is disabled, please enable it firstly.');
        drupal_set_message ( $message, 'warning' );
    }
}

/**
 * Runs the batch process for one entity
 */
function _taxonomy_autotagging_batch_process_single($terms, $instance, $id, $info, &$context) {
    $time_start = microtime(true);
    //get the tokenized vector at first
    $entity = $info['load hook']($id);
    $vector = _taxonomy_autotagging_entity_tokenize($entity);

    //update with the taxonomy autotagging results
    $analysis = _taxonomy_autotagging_analysis($vector, $terms, $instance);
    if (!empty($analysis)) {
        $entity->{$instance['field_name']}['und'] = $analysis;
        field_attach_update($instance['entity_type'], $entity);
        $msg = 'Taxonomy auto tagging now';
    } else {
        $msg = 'nothing found';
    }

    $time_end = microtime(true);
    $time = $time_end - $time_start;

    // Update batch progress information.
    $context['results']['processed']++;
    $context['message'] = 'We are processing with node - '.$id;
    $context['message'].= ' - '.$msg;
    $context['message'].= ' - in '.round($time,2).' second(s)';
}

/**
 * Batch process finished callback.
 */
function _taxonomy_autotagging_batch_finished($success, $results, $operations) {
    if ($success) {
        $message = count ( $results ) . ' processed by Taxonomy Auto Tagging.';
        $message .= theme ( 'item_list', $results );
    } else {
        $error_operation = reset ( $operations );
        $message = t ( 'An error occurred while processing %error_operation with arguments: @arguments', array (
            '%error_operation' => $error_operation [0],
            '@arguments' => print_r ( $error_operation [1], TRUE )
        ) );
    }
    drupal_set_message ( $message );
}
