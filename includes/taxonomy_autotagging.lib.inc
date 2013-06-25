<?php
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
function _taxonomy_autotagging_prepare_terms($field) {
	$terms = array();
	$vocabulary = taxonomy_vocabulary_machine_name_load($field["settings"]["allowed_values"][0]["vocabulary"]);
	$tree = taxonomy_get_tree($vocabulary->vid, 0, NULL, TRUE);
	foreach ($tree as $term) {
		$termlist = array($term->name);
		$synonyms = synonyms_get_term_synonyms($term);
		foreach ($synonyms as $synonym) {
			$termlist[] = $synonym['value'];
		}
		$terms[] = array(
			'tid' => $term->tid,
			'name' => $termlist,
		);
	}
	return $terms;
}