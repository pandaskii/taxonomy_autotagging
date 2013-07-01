<?php
/**
 * Implements hook_drush_command().
 *
 * @return array
 *   An associative array describing your command(s).
 */
function taxonomy_autotagging_drush_command()
{
    $items = array();
    $items['taxonomy-autotagging-all'] = array(
        'callback' => 'taxonomy_autotagging_drush_process_all',
        'description' => dt("Start Taxonomy Auto Tagging process."),
        'arguments' => array(),
        'required-arguments' => FALSE,
        'options' => array(),
        'examples' => array(
            'drush tat' => 'Analysis and process all the nodes.',
        ),
        'aliases' => array('tat'),
    );
    return $items;
}

/**
 * Run full re-analysis
 */
function taxonomy_autotagging_drush_process_all()
{
    $bundles = node_type_get_types();
    foreach ($bundles as $bundle) {
        $field_info = field_info_instances();
        foreach ($field_info['node'][$bundle->type] as $field) {
            if (isset($field['settings']['taxonomy_autotagging_active']) && $field['settings']['taxonomy_autotagging_active'] == '1') {
                $field_name = $field['field_name'];
                $instances[] = field_info_instance('node', $field_name, $bundle->type);
            }
        }
    }

    if (!function_exists('_drush_taxonomy_autotagging')) {
        drush_set_error(dt("The function '@fucname' is not found.", array('@fucname' => '_drush_taxonomy_autotagging')));
    } else {
        if (is_array($instances) && !empty($instances)) {
            foreach ($instances as $instance) {
                drush_print('Processing '.$instance['bundle'].' with Taxonomy field '.$instance ['field_name']);
                _drush_taxonomy_autotagging($instance);
            }
        } else {
            drush_set_error(dt('We cannot find the instance for Taxonomy Auto Tagging.'));
        }
    }

    drush_print(t('Updated Taxonomy Terms matching for all the nodes'));
}

function _drush_taxonomy_autotagging($instance)
{
    // check if taxonomy auto tagging is active
    $taxonomy_autotagging_active = isset($instance['settings']['taxonomy_autotagging_active']) ? $instance['settings']['taxonomy_autotagging_active'] : 0;
    if ($taxonomy_autotagging_active != 1) return;
    if ($taxonomy_autotagging_active == 1) {
        // get all the entity ids from the bundle
        // prepare the ids
        $bundle = $instance ['bundle'];
        $field_name = $instance ['field_name'];
        $entity_results = db_query('select n.nid from {node} n WHERE n.type = :type AND n.nid NOT IN (select entity_id from {taxonomy_autotagging_setting} tan WHERE field_name = :field_name AND status = 0 AND entity_id = n.nid)',
            array(
                ':type' => $bundle,
                ':field_name' => $field_name,
            ));
        $entity_total = $entity_results->rowCount();
        if ($entity_total > 0) {
            $entity_ids = $entity_results->fetchCol();
            // Using Batch API for bulk autocategorising
            $batch = array(
                'title' => t('Taxonomy Auto Tagging'),
                'operations' => array(),
                'finished' => '_drush_taxonomy_autotagging_batch_finished',
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
                    )
                );
            }
            $batch['progressive'] = FALSE;
            batch_set($batch);
            drush_backend_batch_process();
            $batch =& batch_get();
            $batch['current_set']++;
        } else {
            $message = dt('Taxonomy Auto Tagging cannot find the nodes which require the auto tagging, please check if you disable the auotagging with this vocabulary.');
            drupal_set_message($message, 'warning');
            return;
        }
    } else {
        $message = dt('Taxonomy Auto Tagging is disabled, please enable it firstly.');
        drupal_set_message($message, 'warning');
        return;
    }
}

/**
 * Batch process finished callback.
 */
function _drush_taxonomy_autotagging_batch_finished($success, $results, $operations)
{
    if ($success) {
        $message = format_plural(count($results), 'One post processed by Taxonomy Auto Tagging Command.', '@count posts processed.');
        drush_print($message);
    } else {
        // An error occurred.
        // $operations contains the operations that remained unprocessed.
        $error_operation = reset($operations);
        if (!empty($error_operation[0])) {
            $message = t('An error occurred while processing @operation with arguments: @args', array('@operation' => $error_operation[0], '@args' => print_r($error_operation[0], TRUE)));
            drush_print($message);
        }
    }
}