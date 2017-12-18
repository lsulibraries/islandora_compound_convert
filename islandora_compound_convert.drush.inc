<?php

/**
 * Implements hook_drush_command().
 */
function islandora_compound_convert_drush_command() {
  $items = array();

  $items['islandora_compound_convert_item'] = array(
    'aliases' => array('icnsptest'),
    'description' => 'Clone an object into another namespace, optionally deleting the original.',
    'drupal dependencies' => array(
      'islandora',
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_LOGIN,
    'options' => array(
      'pid' => array(
        'description' => "object to move to a new namespace.",
        'value' => 'required',
      ),
      'parent' => array(
        'description' => 'Collection to clone an item into.',
        'value' => 'optional',
      ),
    ),
  );

  return $items;
}

function drush_islandora_compound_convert_item() {
  $pid = drush_get_option('pid');
  $cpd = islandora_object_load($pid);
  $parent = drush_get_option('parent', FALSE);

  if (!$cpd) {
    drush_set_error("Source $pid does not exist!");
    drush_die();
  }
  module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
  $tuque = new IslandoraTuque();
  $repository = $tuque->repository;

  if (!in_array('islandora:compoundCModel', $cpd->models)) {
    drush_set_error("$pid is not a compound object.");
  }

  module_load_include('module', 'islandora_compound_object', 'islandora_compound_object');
  $constituents = islandora_compound_object_get_parts($cpd->id);

  $book_pid = islandora_cc_create_new_book($cpd, $repository, $parent);

  foreach ($constituents as $page) {
    echo "processing $page\n";
    islandora_cc_add_page_to_book($page, $book_pid, str_replace(':', '_', $pid));
  }
}

function islandora_cc_create_new_book($cpd, $repository, $parent) {
  module_load_include('inc', 'islandora', 'includes/IslandoraTuque');
  $book = $repository->constructObject(islandora_get_namespace($cpd));
  $book->label = $cpd->label;
  $book->relationships->add(FEDORA_RELS_EXT_URI, 'isMemberOfCollection', $parent);
  $book->relationships->add(FEDORA_MODEL_URI, 'hasModel', "islandora:bookCModel");
  $book->ingestDatastream($cpd['MODS']);
  islandora_add_object($book);
  return $book->id;
}

function islandora_cc_add_page_to_book($src_obj, $book_pid, $cpdParentPid) {
  global $user;
  module_load_include('inc', 'islandora', 'includes/utilities');
  module_load_include('inc', 'islandora', 'includes/ingest.form');
  $tuque = islandora_get_tuque_connection();
  $page = $tuque->repository->constructObject(islandora_get_namespace($book_pid));
  $page->owner = $user->name;
  $page->models = 'islandora:pageCModel';

  $source_object = islandora_object_load($src_obj);
  $sequence_number = $source_object->relationships->get(ISLANDORA_RELS_EXT_URI, "isSequenceNumberOf$cpdParentPid")[0]['object']['value'];

  $page->label = sprintf("[page %s]", $sequence_number);

  $rels_ext = $page->relationships;
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isPageOf', $book_pid);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', (string) $sequence_number, TRUE);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isPageNumber', (string) $sequence_number, TRUE);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isSection', '1', TRUE);
  islandora_paged_content_set_relationship($rels_ext, FEDORA_RELS_EXT_URI, 'isMemberOf', $book_pid);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'hasLanguage', 'no_ocr', TRUE);

  foreach ($source_object as $dsid => $content) {
    if (!$source_object[$dsid]->content) {
      $msg = "$source_pid: No content found for DSID $dsid in object $source_pid while trying to clone into $new_pid.";
      drush_log($msg, 'warning');
    }
    else {
      if (in_array($dsid, array('MODS', 'OBJ'))) {
        echo "ingesting dsid $dsid\n";
        $page->ingestDatastream($content);
      }
    }
  }
  islandora_add_object($page);
}
