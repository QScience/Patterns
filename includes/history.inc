<?php

/**
 * @file
 * Manipulates pattern files to add information about the history of
 * merging and pulling.
 */

/**
 * Adds a parent tag in the info section of the pattern.
 *
 * Delete previous existing parent tag, and add a timestamp for each parent.
 *
 * Parent must be an array of the type:
 *
 * $parent = array(
 *   'action' => 'merge',
 *   'host' => $url_rpc,
 *   'd2d_id' => $d2d_id,
 *   'uuuid' => $pattern->uuuid,
 *   'title' => $pattern->title,
 *   'category' => @$pattern->pattern['category'],
 *   'author' => $pattern->author,
 * );
 *
 * Title, Category, Author, and D2D id fields are optional.
 *
 * @param array $pattern
 *   Pattern in which the new header will be appended
 * @param array $parents
 *   Array of arrays of parent objects
 *
 * @return Pattern including the new header
 */
function patterns_history_set_parent(&$pattern, $parents) {
  $pattern['info']['parent'] = array();
  foreach ($parents as $p) {
    $p['category'] = isset($p['category']) && !empty($p['category']) ?
      $p['category'] : 'default';
    $p['author'] = isset($p['author']) && !empty($p['author']) ?
      $p['author'] : 'unknown';
    $p['d2d_id'] = isset($p['d2d_id']) && !empty($p['d2d_id']) ?
      $p['d2d_id'] : 'unknown';
    $p['title'] = isset($p['title']) && !empty($p['title']) ?
      $p['title'] : 'no title';

    if (!isset($p['host']) || !isset($p['action']) || !isset($p['uuuid'])) {
      throw new Exception('Missing mandatory attributes in patterns_history_set_parent');
    }

    $p['date'] = date_timestamp_get(date_create());
    $pattern['info']['parent'][] = $p;
  }
}