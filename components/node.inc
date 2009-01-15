<?php

function node_patterns($op, $id = null, &$data = null, $a = null) {
  switch($op) {
    // Return the valid tags that this component can prepare and process
    case 'tags':
      return array('node');
    break;
    
    // Return a list of forms/actions this component can handle
    case 'actions':
      return array(
        $data['type'] .'_node_form' => t('Node: Create/update content.'),
        'node_delete_confirm' => t('Node: Delete content.'), 
      );
    break;
    
    // Return a summary of an action
    case 'summary':
      $variables = array(
        '%type' => $data['type'],
        '%title' => $data['title'],
       );
       if (empty($data['nid'])) {
         return t('Create %type: "%title"', $variables);
       }
       elseif ($data['delete']) {
         return t('Delete %type: "%title"', $variables);
       }
       else {
         return t('Update %type: "%title"', $variables);
       }
    break;
    
    // Prepare data for processing
    case 'prepare':
      global $user;
    
      if (empty($data['nid']) && !empty($data['id'])) {
        $data['nid'] = $data['id'];
      }
      unset($data['id']);

      if (empty($data['name']) && !empty($data['author'])) {
        $data['name'] = $data['author'];
      }
      unset($data['author']);      
      
      if (empty($data['uid'])) $data['uid'] = $user->uid;
      if (empty($data['name'])) $data['name'] = $user->name;

      if (is_numeric($data['nid']) && $data['node'] = node_load($data['nid'])) {
        $data['update'] = TRUE;
      }
      else {
        unset($data['nid']);
        unset($data['vid']);
        $data['node'] = (object) $data;
        $data['update'] = FALSE;
      }          
      
    break;
    
    // Pre validate actions
    case 'pre-validate':
      if (empty($data['type']) && !$data['delete']) {
        return t('"type" field is required.');
      }

      if ($data['delete'] && empty($data['nid'])) {
        return t('"id" field is required.');
      }
      
      if ($data['update'] && $data['type'] != $data['node']->type) {
        return t("You can't change content type for already existing node.");
      }
    break;
    
    // Return the form_id('s) for each action
    case 'form_id':
      if ($data['delete']) {
        return 'node_delete_confirm';
      }
      else {
        return $data['type'] .'_node_form';
      }
    break;
    
    // Prepare for valid processing of this type of component
    case 'build':
      module_load_include('inc', 'node', 'node.pages');

      if ($id == 'node_delete_confirm') {
        $data['confirm'] = 1;
        return $data;
      }

      $data['changed'] = time();
      
      $data['op'] = t('Save');
      $node = drupal_clone($data['node']);
      
      // build CCK fields
      $type = $data['type'];
      $form_id = $type .'_node_form';
      $form_state = array();
      $form = drupal_retrieve_form($form_id, $form_state, $node);
      drupal_prepare_form($form_id, $form, $form_state);
      
      $content_type = content_types($type);
      $fields_info = $content_type['fields']; 
      $groups = fieldgroup_groups($type, FALSE, TRUE);

      // TODO: make this work for PHP4
      $fields = array();
      $fields_diff = array();
      if (!empty($groups)) {
        foreach($groups as $key => $group) {
          $fields = array_merge($fields, array_intersect_ukey($form[$key], $group['fields'], '_patterns_compare_keys')); 
        }
        $fields_diff = array_diff_ukey($fields_info, $fields, '_patterns_compare_keys');
        $fields = array_merge($fields, array_intersect_ukey($form, $fields_diff, '_patterns_compare_keys'));
      }
      else {
        $fields = array_merge($fields, array_intersect_ukey($form, $fields_info, '_patterns_compare_keys'));
      }

      if (!empty($fields)) {
        $defaults = array();
        foreach($fields as $field_name => $field) {
          $name = count($field[0]['#columns']) == 1 ? $field[0]['#columns'][0] : 'value';
          $alias = str_replace('field_', '', $field_name);
           
          if (isset($field['#default_value']) && !isset($field[0])) {
            $default = empty($field['default_value']) ? '' : $field['default_value'];
            $defaults[$field_name][0][$name] = _patterns_get_field_value($data['fields'][$alias], $name, $default);
          }
          else {
            $i = 0;
            while (isset($field[$i])) {          	
              if (is_string($field[$i]['#default_value'])) {
                $default = empty($field[$i]['default_value']) ? '' : $field[$i]['default_value'];
                $defaults[$field_name][$i][$name] = _patterns_get_field_value($data['fields'][$alias], $name, $default);
              }
              elseif (is_array($field[$i]['#default_value'])) {
                foreach ($field[$i]['#default_value'] as $key => $val) {
                  $defaults[$field_name][$i][$key] = _patterns_get_field_value($data['fields'][$alias], $key, $val);
                }
              }
        
              if (count($field[$i]['#columns']) > 1) {
                foreach ($field[$i]['#columns'] as $col) {
                  if (isset($field[$i][$col])) {
                    $value = _patterns_get_field_value($data['fields'][$alias], $col, $field[$i][$col]['#default_value']);
                    $defaults[$field_name][$i][$col] = array($col => $value);
                  }
                  else {
                    $defaults[$field_name][$i][$col] = _patterns_get_field_value($data['fields'][$alias], $col, NULL);
                  }
                }
              }            
              $i++;
            }
          }
        }
        $data = $data + $defaults;
      }

      // build taxonomy array
      $vocabularies = taxonomy_get_vocabularies($data['type']);      
      
      $vocabs = array();
      // we assume that all the vocabularies have unique names
      foreach ($vocabularies as $v) {
        $vocabs[$v->name] = $v;
      }

      $new_taxonomy = array();
      
      // PHP array produced by XML parser when single vocabulary 
      // is defined in the pattern differs from the one produced 
      // when mutiple vocabs are defined.
      // We need to convert single vocab array to the array 
      // structure for multiple vocabs.  
      if (isset($data['taxonomy']['vocabulary'])) {
        $data['taxonomy'] = array(0 => $data['taxonomy']);
      }
      
      if (!empty($data['taxonomy']) && is_array($data['taxonomy'])) {
        foreach ($data['taxonomy'] as $key => $v) {          
          if (!isset($vocabs[$v['vocabulary']])) continue;
            
          $vocabulary = $vocabs[$v['vocabulary']];
          unset($data['taxonomy'][$key]['vocabulary']);
          $terms = $data['taxonomy'][$key];
            
          if (!is_array($terms)) continue;
          
          if ($vocabulary->tags) {
            $new_taxonomy['tags'][$vocabulary->vid] = reset($terms);
          }
          else {
            // get tids for all term names
            $tids = array();
            foreach($terms as $name) {
              if (!is_string($name)) continue;
              
              $term = taxonomy_get_term_by_name($name);
              // take the first term that belongs to given vocabulary
              foreach ($term as $t) {
                if ($t->vid == $vocabulary->vid) { 
                  $tids[$t->tid] = $t->tid;
                  break;
                } 
              }
            }
            $new_taxonomy[$vocabulary->vid] = $vocabulary->multiple ? $tids : (empty($tids) ? '' : reset($tids));
          }
        }
      }

      if (!$data['update'] || ($data['update'] && $data['taxonomy_overwrite'])) {
        if (empty($new_taxonomy)) {
          unset($data['taxonomy']);
        }
        else {
          $data['taxonomy'] = $new_taxonomy;
        }       
      }
      else {
        $taxonomy = array();
        $defaults = array();

        foreach ($form['taxonomy'] as $vid => $terms) {
          if (!is_numeric($vid)) continue;
          
          $tids = array();
          foreach ($terms['#default_value'] as $tid) {
            $tids[$tid] = $tid;
          }
          
          $empty = $terms['#multiple'] ? array() : '';
          $default = $terms['#multiple'] ? $tids : reset($tids);

          if (empty($tids)) {
            $taxonomy[$vid] = empty($new_taxonomy[$vid]) ? $empty : $new_taxonomy[$vid];  
          }
          else {
            if ($terms['#multiple']) {
              $taxonomy[$vid] = empty($new_taxonomy[$vid]) ? $default : $new_taxonomy[$vid] + $default;
            }
            else {
              $taxonomy[$vid] = empty($new_taxonomy[$vid]) ? $default : $new_taxonomy[$vid];
            }
          }
        }
        
        if (!empty($form['taxonomy']['tags'])) {
          foreach ($form['taxonomy']['tags'] as $vid => $tags) {
            $defaults['tags'][$vid] = $tags['#default_value'];
            if (!empty($new_taxonomy['tags'][$vid])) {
              $taxonomy['tags'][$vid] = !empty($defaults['tags'][$vid]) ? $defaults['tags'][$vid] .','. $new_taxonomy['tags'][$vid] : $new_taxonomy['tags'][$vid];  
            }
            else {
              $taxonomy['tags'][$vid] = $defaults['tags'][$vid]; 
            }
          }            
        }
        $data['taxonomy'] = $taxonomy;
      } 

      unset($data['fields']);
      unset($data['node']);
      unset($data['taxonomy_overwrite']);

      $node->taxonomy = array();
      if ($data['update']) $data = array_merge((array)$node, $data);

      unset($data['update']);

      return $data;
    break;
    
    // Validate the values for an action before running the pattern
    case 'validate':
      // TODO: validate name field - should be valid/existing username
      
      $type = content_types($data['type']);
      if ($type['type'] != $data['type'] && !$data['delete']) {
        return t('Invalid content type: %type', array('%type' => $data['type']));
      }
    break;
    
    // Build a patterns actions and parameters
    case 'params':
      return array((object) $data);
    break;

    // Cleanup any global settings after the action runs
    case 'cleanup':
    break;
    
    // Return the primary ID if possible from this action
    case 'identifier':
      $form_state = $a;
      if (!empty($form_state['nid']) && is_numeric($form_state['nid'])) {      
        return $form_state['nid'];
      }
    break;    
  }
}

function _patterns_compare_keys($key1, $key2) {
  if ($key1 == $key2)
    return 0;
  elseif ($key1 > $key2)
    return 1;
  else
    return -1;  
}

function _patterns_get_field_value($field, $key, $default) {
  if (empty($field)) {
    return $default;
  }
  elseif (is_string($field)) {
    return $field;
  }
  elseif (is_array($field)) {
    return empty($field[$key]) ? $default : $field[$key];
  }
}