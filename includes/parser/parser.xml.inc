<?php

// Implements hook patterns_parser_add
function patterns_xmlparser_patterns_parser() {
	$parser = array();
	$parser['format'] = PATTERNS_FORMAT_XML;
	$parser['parser'] = 'patterns_xmlparser'; 
	return $parser;  
}

/**
 * Loads an XML pattern file without validating.
 *
 * @param string $path The path of the pattern file.
 * @param bool $local
 *   (optional) TRUE if the pattern file is local. Defaults to TRUE.
 *
 * @return array|bool
 *   The pattern structure, or FALSE.
 */
function patterns_xmlparser_load($path, $local = TRUE) {
  if ($local && !file_exists($path)) {
    return FALSE;
  }

  if (!$xml = file_get_contents($path)) {
    return FALSE;
  }

  return patterns_xmlparser_parse($xml);
}

/**
 * Creates a pattern from an XML data source.
 *
 * @param string $xml The XML string of the data source.
 *
 * @return array|bool
 *   The pattern structure, or FALSE.
 */
function patterns_xmlparser_parse($xml) {
  $parse = drupal_xml_parser_create($xml);
  xml_parser_set_option($parse, XML_OPTION_CASE_FOLDING, 0);

  if (!xml_parse_into_struct($parse, $xml, $vals, $index)) {
    // TODO: Report parsing or other error.
    return FALSE;
  }

  // Create a multi-dimensional array representing the XML structure.
  $pattern = current(_patterns_xmlparser_parse_tag($vals));
  
  if (empty($pattern) || $pattern['xml_tag'] != 'pattern') {
    // TODO: Error reporting.
    return FALSE;
  }
  
  // Rearrange the data in a nice way for each component.
  // Make sure actions are processed differently so order is preserved.
  $pattern = patterns_xmlparser_rearrange_data($pattern);

  return $pattern;
}

/**
 * Takes an array representing a pattern and returns
 * the XML representation of it.
 * 
 * @param array $pattern The pattern array to convert
 * 	to XML.
 */
function patterns_xmlparser_dump($pattern = NULL) {
	if (is_null($pattern) || empty($pattern)) return FALSE;
	
	$xml = new SimpleXMLElement('<pattern/>');
	array_walk_recursive($pattern, array ($xml, 'addChild'));
	return $xml->asXML();
}

// Helping Functions

function patterns_xmlparser_rearrange_data($pattern) {
  foreach ($pattern as $key => $value) {
    if (is_string($key)) {
      unset($pattern[$key]);
    }
    else {
      
     if ($value['xml_tag'] == PATTERNS_SECTION_INFO) {
        $pattern[PATTERNS_SECTION_INFO] = _patterns_xmlparser_rearrange_data($value);
        unset($pattern[PATTERNS_SECTION_INFO]['xml_tag']);
        unset($pattern[$key]); // TODO: unset in _patterns_xmlparser_rearrange_data
     }
     elseif ($value['xml_tag'] == PATTERNS_SECTION_MODULES) {
        $pattern[PATTERNS_SECTION_MODULES] = _patterns_xmlparser_rearrange_data($value);
        unset($pattern[PATTERNS_SECTION_MODULES]['xml_tag']);
        unset($pattern[$key]); // TODO: unset in _patterns_xmlparser_rearrange_data
     }
     
     // Normal section
     else {
        $section = $value['xml_tag']; 
        $pattern[$section] = array();
        unset($value['xml_tag']);
        foreach($value as $action) { 
          $tag = array('tag' => @$action['tag']); // TODO: check when this does not exist
          $_action = $action['xml_tag']; // TODO: check that is a valid one and that exist
          //$pattern[$section][] = array($_action => array_merge($tag, _patterns_xmlparser_rearrange_data($action)));
          $_action_data = _patterns_xmlparser_rearrange_data($action);
          unset($_action_data['xml_tag']);
          $pattern[$section][] = array($_action => $_action_data);
        }
        unset($pattern[$key]);
      }
     
    }
  }

  return $pattern;
}

function _patterns_xmlparser_rearrange_data($data, $parent = '') {
  $numeric = array();
  $count = 0;
  
  foreach ($data as $key => $value) {
    if (isset($value['value'])) {
      if ($value['value'] == 'false') {
        $value['value'] = FALSE;
      }
      elseif ($value['value'] == 'true') {
        $value['value'] = TRUE;
      }
    }

    if (is_numeric($key) && is_array($value) && count($value) == 2 && isset($value['tag']) && isset($value['value'])) {
      unset($data[$key]);
      if (isset($data[$value['xml_tag']])) {
        $numeric[] = $value['xml_tag'];
        $data[$count++] = $data[$value['xml_tag']];
        $data[$count++] = $value['value'];
        unset($data[$value['xml_tag']]);
      }
      elseif (in_array($value['tag'], $numeric)) {
        $data[$count++] = $value['value'];
      }
      else {
        $data[$value['xml_tag']] = $value['value'];
      }
    }
    elseif (is_numeric($key)) {
      $tag = $value['xml_tag'];
      unset($value['xml_tag']);
      $value = _patterns_xmlparser_rearrange_data($value, $tag);
      if (isset($value['value'])) {
//        debug($value);
//      }
//      else {
          $data[$tag] = $value['value'];
      }
      unset($data[$key]);
    }
  }

  foreach ($data as $key => $value) {
    if (is_array($value) && count($value) == 1 && $value[0]) {
      $data[$key] = $data[$key][0];
    }
  }

  // This workaround enables us to define numeric keys in XML by
  // prefixing the number with single character. E.g <n0>value</n0>
  // will result in 0 => 'value' (first character of the key will be removed).
  if (isset($data['_numeric_keys'])) {
    unset($data['_numeric_keys']);
    foreach ($data as $key => $value) {
      $data[substr($key, 1)] = $value;
      unset($data[$key]);
    }
  }
  
  return $data;
 
}

/**
 * Recurse through the values of a parsed xml file to create a
 * multi-dimensional representation of the data.
 */
function _patterns_xmlparser_parse_tag($data, &$index = 0) {
  $pattern = array();

  while (isset($data[$index]) && $current = $data[$index]) {
    $type = $current['type'];

    // Changing tag into xml_tag,
    // otherwise it conflicts with the key 'tag' of the pattern
    $current['xml_tag'] = $current['tag'];
    unset($current['tag']);
     
    if (!empty($current['attributes'])) {
      foreach ((array) $current['attributes'] as $key => $value) {
        $current[$key] = $value;
      }
    }

    unset($current['type'], $current['level'], $current['attributes']);

    if (isset($current['value']) && !trim($current['value']) && $current['value'] != "0") {
      unset($current['value']);
    }

    switch ($type) {
      case 'open':
        $index++;
        $current += _patterns_xmlparser_parse_tag($data, $index);
        $pattern[] = $current;
        break;
      case 'close':
        $index++;
        return $pattern;
        break;
      case 'complete':
        // In order to support more complex/non-standard features we can use serialized data
        if (!empty($current['attributes']['serialized'])) {
          $value = unserialize($current['value']);

          if (isset($value)) {
            $current['value'] = $value;
          }
        }

        // If no value was specified, make sure an empty value is there
        if (!isset($current['value'])) {
          $current['value'] = '';
        }

        $pattern[] = $current;
        break;
    }

    $index++;
  }

  return $pattern;
}