<?php

function redis_sync_admin_form() {
  $form = array();
  $ets = entity_get_info();
  foreach ($ets as $type => $et) {
    $form[$type . '_wrapper'] = array(
      '#type' => 'fieldset',
      '#title' => $et['label'],
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    $form[$type . '_wrapper']['redis_sync_enable_' . $type] = array(
      '#type' => 'checkbox',
      '#title'  => t('Enable @entity_type synchronisation to redis storage', array('@entity_type' => strtolower($et['label']))),
      '#default_value' => variable_get('redis_sync_enable_' . $type),
    );
  }
  if (variable_get('redis_sync_enable_node')) {
    $cts = node_type_get_types();
    $form['node_wrapper']['content_types_wrapper'] = array(
      '#type' => 'fieldset',
      '#title' => t('Content types'),
      '#weight' => 5,
      '#collapsible' => FALSE,
    );
    foreach ($cts as $ct) {
      $form['node_wrapper']['content_types_wrapper']['redis_sync_enable_node_' . $ct->type] = array(
        '#type' => 'checkbox',
        '#title' => t('Enable synchronisation "@type" to redis storage', array('@type' => $ct->name)),
        '#default_value' => variable_get('redis_sync_enable_node_' . $ct->type),
      );
    }

  }
  return system_settings_form($form);
}


function redis_sync_admin_resync_form() {
  $form = array();

  $form['redis_sync_mode_wrapper'] = array(
    '#type' => 'fieldset',
    '#title' => t('Sync mode'),
    '#collapsible' => FALSE,
  );

  $form['redis_sync_mode_wrapper']['redis_sync_mode'] = array(
    '#type' => 'radios',
    '#title' => t('Sync mode'),
    '#default_value' => variable_get('redis_sync_mode', 1),
    '#options' => array(0 => 'Force', 1 => 'Smart'),
    '#description' => t('ATTENTION: Force mode will remove ALL data from your storage. Smart mode will add missing data.'),
  );


  $ets = entity_get_info();
  foreach ($ets as $type => $et) {
    $form[$type . '_wrapper'] = array(
      '#type' => 'fieldset',
      '#title' => $et['label'],
      '#collapsible' => TRUE,
      '#collapsed' => FALSE,
    );
    if ($type != 'node') {
      $form[$type . '_wrapper']['redis_resync_' . $type] = array(
        '#type' => 'checkbox',
        '#title' => t('Re-sync @entity_type to redis storage', array('@entity_type' => strtolower($et['label']))),
        '#return_value' => array('entity_type' => $type),
      );
    }
    else {
      $cts = node_type_get_types();
      $form[$type . '_wrapper']['content_types_wrapper'] = array(
        '#type' => 'fieldset',
        '#title' => t('Content types'),
        '#collapsible' => FALSE,
      );
      foreach ($cts as $ct) {
        $form[$type . '_wrapper']['content_types_wrapper']['redis_resync_' .$type . '_' . $ct->type] = array(
          '#type' => 'checkbox',
          '#title' => t('Re-sync "@type" to redis storage', array('@type' => $ct->name)),
          '#return_value' => array('entity_type' => $type, 'entity_bundle' => $ct->type),
        );
      }
    }
  }

  $form['redis_sync_start_sync'] = array(
    '#type' => 'submit',
    '#value' => t('Start sync'),
  );

  $query = new EntityFieldQuery;


    $query->entityCondition('entity_type', 'node')
      ->entityOrderBy('entity_id');

  $query = $query->entityCondition('bundle', 'article');

  $result = $query->execute();

  dpm($result);

  return $form;
}

function redis_sync_admin_resync_form_submit(&$form, &$form_state) {
  $trash = array(
    'form_build_id' => 1,
    'form_token' => 1,
    'form_id' => 1,
    'op' => 1,
    'redis_sync_start_sync' => 1,
    'redis_sync_mode' => 1,
  );

  $sync_mode = $form_state['values']['redis_sync_mode'];
  $sync_values = (array_diff_key($form_state['values'], $trash));
  $operations = array();

  if (!empty($sync_values)) {
    foreach ($sync_values as $entity) {
      if (!empty($entity)) {
        $entity_type = $entity['entity_type'];
        $entity_bundle = !empty($entity['entity_bundle']) ? $entity['entity_bundle'] : NULL;
        $operations[] = array('redis_sync_resync', array($entity_type, $entity_bundle));
      }
    }
  }

  if (!$sync_mode) {
    $client = redis_sync_get_client();
    $client->flushdb();
  }

  $batch = array(
    'operations' => $operations,
    'finished' => 'redis_sync_admin_batch_finished',
    'file' => drupal_get_path('module', 'redis_sync') . '/redis_sync.admin.inc',
  );

  batch_set($batch);
}

function redis_sync_resync($entity_type, $entity_bundle, &$context) {
  $count_query = new EntityFieldQuery;
  $count_query->entityCondition('entity_type', $entity_type)

    ->entityOrderBy('entity_id')
    ->entityCondition('entity_id', 0, '!=')
    ->count();

  if ($entity_bundle) {
    $count_query = $count_query->entityCondition('bundle', $entity_bundle);
  }

  $count = $count_query->execute();

  if (empty($context['sandbox'])) {
    $context['sandbox']['progress'] = 0;
    $context['sandbox']['prev_entity_id'] = 0;
    $context['sandbox']['max'] = $count;
  }

  $query = new EntityFieldQuery;
  $limit = 5;

  $query->entityCondition('entity_type', $entity_type)
    ->entityCondition('entity_id', $context['sandbox']['prev_entity_id'], '>')
    ->entityOrderBy('entity_id')
    ->range(0, $limit);

  if ($entity_bundle) {
    $query = $query->entityCondition('bundle', $entity_bundle);
  }

  $result = $query->execute();

  if (!empty($result)) {
    foreach ($result as $entities) {
      foreach ($entities as $entity_id => $value) {
        $entity = entity_load($entity_type, array($entity_id));
        $entity = array_shift($entity);

        $key = _redis_sync_hash_key($entity, $entity_type);
        $hash = _redis_sync_hash($entity);
        $client = redis_sync_get_client();
        $value = $client->hgetall($key);
        if (empty($value)) {
          redis_sync_push($key, $hash);
        }

        $context['sandbox']['progress']++;
        $context['sandbox']['prev_entity_id'] = $entity_id;
        $context['results'][] = $entity_id;
        $context['message'] = $entity_type . ':' . $entity_id;
      }
    }
  }

  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }

}

function redis_sync_admin_batch_finished($success, $results, $operations) {
  if ($success) {
    $message = count($results) .' items processed.';
    drupal_set_message($message);
  }
  else {
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array('%error_operation' => $error_operation[0], '@arguments' => print_r($error_operation[1], TRUE)));
    drupal_set_message($message, 'error');
  }
}

function redis_sync_admin_connection_form() {
  $form = array();

  $form['redis_sync_scheme'] = array(
    '#type' => 'textfield',
    '#title' => t('Scheme'),
    '#description' => t('Connection scheme. Only tcp is currently supported.'),
    '#default_value' => variable_get('redis_sync_scheme', 'tcp'),
    '#disabled' => TRUE,
  );

  $form['redis_sync_host'] = array(
    '#type' => 'textfield',
    '#title' => t('Host'),
    '#description' => t('Redis server host. Default is 127.0.0.1.'),
    '#default_value' => variable_get('redis_sync_host', '127.0.0.1'),
  );

  $form['redis_sync_port'] = array(
    '#type' => 'textfield',
    '#title' => t('Port'),
    '#description' => t('Redis server port. Default is 6379.'),
    '#default_value' => variable_get('redis_sync_port', 6379),
  );

  $form['redis_sync_database'] = array(
    '#type' => 'textfield',
    '#title' => t('Database'),
    '#description' => t('Redis server database. Default is none, Redis server will autoselect the database 0.'),
    '#default_value' => variable_get('redis_sync_database', 0),
  );

  return system_settings_form($form);
}