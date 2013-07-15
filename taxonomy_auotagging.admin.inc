<?php

/**
 * @file
 * Taxonomy Auto Tagging admin page.
 */

/**
 * Page callback that shows an overview.
 */
function taxonomy_autotagging_admin_overview() {
  $rows = array();
  $header = array();
  $header[] = t('Status');
  $header[] = array('data' => t('Type'), 'colspan' => 2);
  $header[] = t('Name');
  $header[] = array('data' => t('Operations'), 'colspan' => 3);
  
  return array(
  		'#theme' => 'table',
  		'#header' => $header,
  		'#rows' => $rows,
  		'#empty' => t('There are no taxonomy auto tagging enabled yet.'),
  );
}
