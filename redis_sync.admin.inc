<?php

function redis_sync_admin_form() {
  $form = array();
  $ets = entity_get_info();
  foreach ($ets as $type => $et) {
    $form[$type . '_wrapper'] = array(
      '#type' => 'fieldset',
      '#title' => $et['label'],
      '#weight' => 5,
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
  // TODO: Implement it.
  $form = array();


  return $form;
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
