<?php

/**
 * 
 * Checks the existance of previous parents and prepends the
 * given one at the beginning.
 * 
 * @param array $pattern  In an array format, as received from the parser
 * @param string $uuuid  Parent UUUID
 * @return array Pattern with the new parent prepended
 */
function patterns_parenthood_add_parent($pattern, $uuuid) {
  if (isset($pattern['info']['parents'])) {
    array_unshift($pattern['info']['parents'], $uuuid);
  }else{
    $pattern['info']['parents'][] = $uuuid;
  }
  return $pattern;
}

function patterns_parenthood_add_header($pattern, $action, $host, $ancestors) {
  /*if (isset($pattern['info']['parents'])) {
    array_unshift($pattern['info']['parents'], $uuuid);
  }else{
    $pattern['info']['parents'][] = $uuuid;
  }*/
  $pattern['info']['history'][$action] = array();
  $pattern['info']['history'][$action]['host'] = $host;
  $pattern['info']['history'][$action]['date'] = date_timestamp_get(date_create());
  foreach ($ancestors as $uuuid) {
    $pattern['info']['history'][$action]['uuuids'][] = $uuuid;
  }
  
  
  return $pattern;
}