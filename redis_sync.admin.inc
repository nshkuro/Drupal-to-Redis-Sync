<?php

function redis_sync_admin_form() {
  $form = array();
  $ets = entity_get_info();
  foreach ($ets as $type => $et) {
    $entity_info = entity_get_info($type);
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

    if (variable_get('redis_sync_enable_' . $type)) {
      $form[$type . '_wrapper']['redis_sync_key_pattern_wrapper'] = array(
        '#type' => 'fieldset',
        '#title' => t('Hash key configuration'),
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
      );
      $description = t("You can define hash keys with tokens. By default key looks like 'entity_type:entity_id', for example 'node:25' or 'user:1'.
        Install token module for viewing all available patterns.");
      if (module_exists('token')) {
        $form[$type . '_wrapper']['redis_sync_key_pattern_wrapper'] ['token_tree'] = array(
          '#type' => 'fieldset',
          '#title' => t('Replacement patterns'),
          '#collapsible' => TRUE,
          '#collapsed' => TRUE,
          '#description' => theme('token_tree', array('token_types' => array($type))),
          '#weight' => 10,
        );
        $description = t("You can define hash keys with tokens. By default key looks like 'entity_type:entity_id', for example 'node:25' or 'user:1'.");
      }
      $form[$type . '_wrapper']['redis_sync_key_pattern_wrapper']['redis_sync_key_pattern_' . $type] = array(
        '#type' => 'textfield',
        '#title' => t('Hash key pattern'),
        '#description' => $description,
        '#default_value' => variable_get('redis_sync_key_pattern_' . $type, 'entity_type:entity_id'),
      );

      if (!empty($entity_info['bundles']) && (count($entity_info['bundles']) > 1 || !empty($entity_info['bundle keys']))) {
        foreach ($entity_info['bundles'] as $bundle_key => $bundle_value) {
          $form[$type . '_wrapper']['bundle_wrapper'][$type . '_' . $bundle_key . '_wrapper'] = array(
            '#type' => 'fieldset',
            '#title' => $bundle_value['label'],
            '#weight' => 5,
            '#collapsible' => FALSE,
          );

          $form[$type . '_wrapper']['bundle_wrapper'][$type . '_' . $bundle_key . '_wrapper']['redis_sync_enable_' . $type . '_' . $bundle_key] = array(
            '#type' => 'checkbox',
            '#title' => t('Enable synchronisation "@type" to redis storage', array('@type' => $bundle_value['label'])),
            '#default_value' => variable_get('redis_sync_enable_' . $type . '_' . $bundle_key),
          );

          $fields = field_info_instances($type, $bundle_key);

          if (!empty($fields)) {
            $form[$type . '_wrapper']['bundle_wrapper'][$type . '_' . $bundle_key . '_wrapper']['fields_wrapper'] = array(
              '#type' => 'fieldset',
              '#title' => $bundle_value['label'] . ' fields',
              '#weight' => 5,
              '#collapsible' => TRUE,
              '#collapsed' => TRUE,
            );
            foreach ($fields as $field) {
              $form[$type . '_wrapper']['bundle_wrapper'][$type . '_' . $bundle_key . '_wrapper']['fields_wrapper']['redis_sync_enable_' . $type . '_' . $bundle_key . '_' . $field['field_name']] = array(
                '#type' => 'checkbox',
                '#title' => t('"@field"', array('@field' => $field['label'])),
                '#default_value' => variable_get('redis_sync_enable_' . $type . '_' . $bundle_key . '_' . $field['field_name'], 1),
              );
            }
          }
        }
      }
      else {
        $fields = field_info_instances($type, $type);
        $bundle_key = $type;
        if (!empty($fields)) {
          $form[$type . '_wrapper']['bundle_wrapper'][$type . '_' . $bundle_key . '_wrapper']['fields_wrapper'] = array(
            '#type' => 'fieldset',
            '#title' => $et['label'] . ' fields',
            '#weight' => 5,
            '#collapsible' => TRUE,
            '#collapsed' => TRUE,
          );
          foreach ($fields as $field) {
            $form[$type . '_wrapper']['bundle_wrapper'][$type . '_' . $bundle_key . '_wrapper']['fields_wrapper']['redis_sync_enable_' . $type . '_' . $bundle_key . '_' . $field['field_name']] = array(
              '#type' => 'checkbox',
              '#title' => t('"@field"', array('@field' => $field['label'])),
              '#default_value' => variable_get('redis_sync_enable_' . $type . '_' . $bundle_key . '_' . $field['field_name'], 1),
            );
          }
        }
      }
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
    if (variable_get('redis_sync_enable_' . $type)) {
      $form[$type . '_wrapper'] = array(
        '#type' => 'fieldset',
        '#title' => $et['label'],
        '#collapsible' => TRUE,
        '#collapsed' => FALSE,
      );

      $entity_info = entity_get_info($type);

      if (!empty($entity_info['bundles']) && (count($entity_info['bundles']) > 1 || !empty($entity_info['bundle keys']))) {
        $form[$type . '_wrapper']['bundle_wrapper'] = array(
          '#type' => 'fieldset',
          '#title' => t('Bundles'),
          '#collapsible' => FALSE,
        );
        foreach ($entity_info['bundles'] as $bundle_key => $bundle_value) {
          if (variable_get('redis_sync_enable_' . $type . '_' . $bundle_key)) {
            $form[$type . '_wrapper']['bundle_wrapper']['redis_resync_' . $type . '_' . $bundle_key] = array(
              '#type' => 'checkbox',
              '#title' => t('Re-sync "@type" to redis storage', array('@type' => $bundle_value['label'])),
              '#return_value' => serialize(array('entity_type' => $type, 'entity_bundle' => $bundle_key)),
            );
          }
        }
      }
      else {
        $form[$type . '_wrapper']['redis_resync_' . $type] = array(
          '#type' => 'checkbox',
          '#title' => t('Re-sync @entity_type to redis storage', array('@entity_type' => strtolower($et['label']))),
          '#return_value' => serialize(array('entity_type' => $type)),
        );
      }
    }
  }

  $form['redis_sync_start_sync'] = array(
    '#type' => 'submit',
    '#value' => t('Start sync'),
  );

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
        $entity = unserialize($entity);
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
        $hash = _redis_sync_hash($entity, $entity_type);
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

  $form['redis_sync_connection'] = array(
    '#type' => 'fieldset',
    '#title' => t('Connection'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['redis_sync_connection']['redis_sync_scheme'] = array(
    '#type' => 'textfield',
    '#title' => t('Scheme'),
    '#description' => t('Connection scheme. Only tcp is currently supported.'),
    '#default_value' => variable_get('redis_sync_scheme', 'tcp'),
    '#disabled' => TRUE,
  );

  $form['redis_sync_connection']['redis_sync_host'] = array(
    '#type' => 'textfield',
    '#title' => t('Host'),
    '#description' => t('Redis server host. Default is 127.0.0.1.'),
    '#default_value' => variable_get('redis_sync_host', '127.0.0.1'),
  );

  $form['redis_sync_connection']['redis_sync_port'] = array(
    '#type' => 'textfield',
    '#title' => t('Port'),
    '#description' => t('Redis server port. Default is 6379.'),
    '#default_value' => variable_get('redis_sync_port', 6379),
  );

  $form['redis_sync_connection']['redis_sync_database'] = array(
    '#type' => 'textfield',
    '#title' => t('Database'),
    '#description' => t('Redis server database. Default is none, Redis server will autoselect the database 0.'),
    '#default_value' => variable_get('redis_sync_database', 0),
  );

  $form['redis_sync_connection']['redis_sync_password'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
    '#description' => t('Redis server password.'),
    '#default_value' => variable_get('redis_sync_password', ''),
  );

  $form['redis_sync_connection']['redis_sync_pubsub_channel'] = array(
    '#type' => 'textfield',
    '#title' => t('Pub/Sub Channel'),
    '#description' => t('Redis pub/sub channel.'),
    '#default_value' => variable_get('redis_sync_pubsub_channel', ''),
  );

  $openssl = !function_exists('openssl_encrypt');

  $form['redis_sync_encryption'] = array(
    '#type' => 'fieldset',
    '#title' => t('Encryption'),
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
  );

  $form['redis_sync_encryption']['redis_sync_encryption_enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable encryption'),
    '#default_value' => variable_get('redis_sync_encryption_enabled'),
    '#disabled' => $openssl,
  );

  $form['redis_sync_encryption']['redis_sync_encryption_method'] = array(
    '#type' => 'textfield',
    '#title' => t('Encryption method'),
    '#description' => t('OpenSSL encryption method. By default AES 128 CBC'),
    '#default_value' => variable_get('redis_sync_encryption_method', 'aes-128-cbc'),
    '#disabled' => $openssl,
  );

  $form['redis_sync_encryption']['redis_sync_encryption_key'] = array(
    '#type' => 'textfield',
    '#title' => t('Encryption key'),
    '#description' => t("Please make sure that your value is correct. For example, for 128/256 bit AES encryption key size should be of 16/32 bytes (128/256 bits)"),
    '#default_value' => variable_get('redis_sync_encryption_key', '1234567890123456'),
    '#maxlength' => 32,
    '#disabled' => $openssl,
  );
  $form['redis_sync_encryption']['redis_sync_encryption_iv'] = array(
    '#type' => 'textfield',
    '#title' => t('Initialization vector'),
    '#description' => t('Please make sure that your value is correct. For example, for 128/256 bit AES encryption, initialization vector (iv) size should be 16/32 bytes'),
    '#default_value' => variable_get('redis_sync_encryption_iv', '1234567890123456'),
    '#maxlength' => 32,
    '#disabled' => $openssl,
  );
  return system_settings_form($form);
}

function redis_sync_admin_connection_form_validate(&$form, &$form_state) {
  if ($form_state['values']['redis_sync_encryption_enabled']) {
    $iv = $form_state['values']['redis_sync_encryption_iv'];
    $method = $form_state['values']['redis_sync_encryption_method'];
    $iv_length = openssl_cipher_iv_length($method);
    if ($iv_length != strlen($iv)) {
      form_set_error('redis_sync_encryption_iv', t('Length of initialization vector is incorrect'));
    }
  }
}