<?php
/**
 * renders the entity into one string and strips the tags
 */
function _taxonomy_autotagging_entity_tokenize($entity) {
    // run drupal function to get the plain text
    $string = $entity->body['und'][0]['value'];
    //$string = drupal_html_to_text($string);
    return $string;
    //return preg_split('/((^\p{P}+)|(\p{P}*\s+\p{P}*)|(\p{P}+$))/', $string, -1, PREG_SPLIT_NO_EMPTY);
}

/**
 * @param string $vector
 * @param array $terms
 * @param object $instance
 * @return array
 */
function _taxonomy_autotagging_analysis($vector, $terms, $instance) {
	$fl_array = array();
	foreach ($terms as $term) {
		foreach ($term['name'] as $key) {
			if (preg_match("/\b(". $key . "[ieds]*)\b/i", $vector, $matches)) {
				$fl_array[] = array(
						'tid' => $term['tid'],
						'mapping' => $matches[0],
				);
			}
		}
	}
	return $fl_array;
}


/**
 * @param array/object $field
 * @return array $terms
 */
function _taxonomy_autotagging_prepare_terms($field_name, $synonyms_enabled) {
    $field = field_info_field($field_name);
	$terms = array();
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
		$terms[] = array(
			'tid' => $term->tid,
			'name' => $terms,
		);
	}
	return $terms;
}