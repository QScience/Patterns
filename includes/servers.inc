<?php
/**
 * @file
 * Functions related to importing and publishing patterns on the web.
 */

/**
 * Form validation handler for patterns_import_server().
 *
 * @see patterns_import_server_submit()
 */
function patterns_import_server_submit($form, &$form_state) {

  if ($form_state['storage']['step'] == 'get url') {
    return;
  }

  $pids = array_keys(array_filter($form_state['values']['patterns']));
  $feed = $form_state['storage']['feed'];
  $errors = array();

  foreach ($pids as $pid) {
    $url = $feed['info']['url'] . '/admin/patterns/get/' . $pid;
    $pattern_info = $feed['patterns'][$pid];

    if (!$source = file_get_contents($url)) {
      $errors['get file'][] = $pattern_info;
      continue;
    }

    // real_path(public) returns the path to the files dir
    $path = drupal_realpath('public://') . "/" . variable_get('patterns_save_file', 'patterns');
    $path .= '/' . $pattern_info['name'] . '.' . $pattern_info['file_type'];
    if (!$saved = file_save_data($source, $path, FILE_EXISTS_ERROR)) {
      $errors['save file'][] = $pattern_info;
      continue;
    }

    // Choose appropriate function based on the file extension.
    $func = 'patterns_load_' . $pattern_info['file_type'];

    // Load pattern.
    if (!$pattern = $func($saved)) {
      $errors['load pattern'][] = $pattern_info;
      unlink($saved);
      continue;
    }

    patterns_save_pattern($pattern, $saved, $pattern_info['name']);
  }

  if (!empty($errors)) {
    $patterns = array();
    foreach ($errors as $type => $files) {
      foreach ($files as $file) {
        $patterns[] = $file['title'] . ' (cause: "' . $type . '" failed)'; // TODO: t()
      }
      $patterns = theme('item_list', array('items' => $patterns)); // TODO: ???
    }
    drupal_set_message(t('Import failed for the following patterns:') . '<br/>' . $patterns, 'error'); // TODO: proper t()
    return;
  }

  unset($form_state['storage']);
  $form_state['redirect'] = 'admin/patterns';

}

/**
 * Callback function for route /admin/patterns/publish/
 *
 * Sets the flag public to TRUE in the database and invokes the
 * hook_patterns_publish in all the modules implementing it.
 *
 * Redirects the user to the patterns home.
 *
 * @param mixed $pattern the id or the name of the pattern.
 */
function patterns_publish_pattern($pattern) {

  $pattern = patterns_utils_if_invalid_go_back($pattern);
  $result = patterns_db_publish_pattern($pattern);

  if ($result) {
    drupal_set_message(t('Pattern %pattern was marked as public.', array('%pattern' => @$pattern->name)));
  }
  else {
    drupal_set_message(t('An error occurred. Unable to mark pattern %pattern as public.', array('%pattern' => @$pattern->name)), 'error');
  }

  // Patterns API
  // Invoke all the modules who wants to do something with the published pattern
  module_invoke_all('patterns_publish', $pattern);

  drupal_goto('admin/patterns');
}

/**
 * Callback function for route /admin/patterns/unpublish/
 *
 * Sets the flag public to FALSE in the database and invokes the
 * hook_patterns_unpublish in all the modules implementing it.
 *
 * Redirects the user to the patterns home.
 *
 * @param mixed $pattern the id or the name of the pattern.
 */
function patterns_unpublish_pattern($pattern) {
  $pattern = patterns_utils_if_invalid_go_back($pattern);
  $result = patterns_db_unpublish_pattern($pattern);

  if ($result) {
    drupal_set_message(t('Pattern %pattern was marked as private.', array('%pattern' => @$pattern->name)));
  }
  else {
    drupal_set_message(t('An error occurred. Unable to mark pattern %pattern as private.', array('%pattern' => @$pattern->name)), 'error');
  }
  // Patterns API
  // Invoke all the modules who wants to do something with the unpublished pattern
  module_invoke_all('patterns_unpublish', $pattern);

  drupal_goto('admin/patterns');
}
