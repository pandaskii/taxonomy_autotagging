<?php
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
        $vector = implode(' ',$string);
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
 * return the terms array/synonyms if the synonyms module is enabled.
 */
function _taxonomy_autotagging_prepare_terms($field_name, $synonyms_enabled) {
    $terms = array();
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
		$terms[] = array(
			'tid' => $term->tid,
			'name' => $terms,
		);
	}
	return $terms;
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