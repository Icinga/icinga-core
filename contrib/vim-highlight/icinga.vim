" Vim syntax file
" Filename:     icinga.vim
" Language:     icinga template object configuration file (based on nagios.vim by the following authors)
" Author:       Elan Ruusamäe <glen@pld-linux.org>
" Author:       Lance Albertson <ramereth@gentoo.org>
" Author:       Ava Arachne Jarvis <ajar@katanalynx.dyndns.org>
" Maintainer:   Lars Engels <lars.engels@0x20.net>
" Last Change:  2013-11-12 13:45

if !exists("main_syntax")
  if version < 600
    syntax clear
  elseif exists("b:current_syntax")
    finish
  endif

  let main_syntax = 'icinga'
endif

if version >= 600
  setlocal iskeyword=_,-,A-Z,a-z,48-57
else
endif

syn match icingaLineComment '#.*$'
syn match icingaComment '[;#].*$' contained

syn match icingaConstant '\<[0-9]\+%\?\>'
syn match icingaConstant '\<[a-z]\>'

syn region icingaString  start=+"+ end=+"+ contains=icingaMacro
syn region icingaString  start=+'+ end=+'+ contains=icingaMacro

syn match icingaDef 'define[ \t]\+\(\(host\|service\)extinfo\|host\|service\|timeperiod\|contact\|command\)'
syn match icingaDef 'define[ \t]\+\(host\|contact\|service\)group'
syn match icingaDef 'define[ \t]\+\(service\|host\)dependency'
syn match icingaDef 'define[ \t]\+\(service\|host\|hostgroup\)escalation'

syn match icingaMacro  '\$CONTACT\(NAME\|ALIAS\|EMAIL\|PAGER\)\$'
syn match icingaMacro  '\$HOST\(NAME\|ALIAS\|ADDRESS\|STATE\|OUTPUT\|PERFDATA\|STATETYPE\|EXECUTIONTIME\)\$'
syn match icingaMacro  '\$\(ARG\|USER\)\([1-9]\|[1-2][0-9]\|3[0-2]\)\$'
syn match icingaMacro  '\$SERVICE\(DESC\|STATE\|OUTPUT\|PERFDATA\|LATENCY\|EXECUTIONTIME\|STATETYPE\)\$'
syn match icingaMacro  '\$\(OUTPUT\|PERFDATA\|EXECUTIONTIME\|LATENCY\)\$'
syn match icingaMacro  '\$NOTIFICATION\(TYPE\|NUMBER\)\$'
syn match icingaMacro  '\$\(\(SHORT\|LONG\)\?DATETIME\|DATE\|TIME\|TIMET\)\$'
syn match icingaMacro  '\$LASTSTATECHANGE\$'
syn match icingaMacro  '\$ADMIN\(EMAIL\|PAGER\)\$'
syn match icingaMacro  '\$\(SERVICE\|HOST\)ATTEMPT\$'
syn match icingaMacro  '\$LAST\(HOST\|SERVICE\)CHECK\$'

syn region icingaDefBody start='{' end='}'
	\ contains=icingaComment,icingaDirective,icingaMacro,icingaConstant,icingaString,icingaSpecial transparent

syn keyword icingaDirective contained 2d_coords 3d_coords action_url active_checks_enabled address alias can_submit_commands check_command
syn keyword icingaDirective contained check_freshness check_interval check_period checks_enabled command_line command_name contact_groups contact_name
syn keyword icingaDirective contained contactgroup_name contacts dependent_host_name dependent_service_description email event_handler
syn keyword icingaDirective contained event_handler_enabled execution_failure_criteria execution_failure_options failure_prediction_enabled
syn keyword icingaDirective contained first_notification flap_detection_enabled freshness_threshold friday high_flap_threshold host_name
syn keyword icingaDirective contained host_notification_commands host_notification_options host_notification_period host_notifications_enabled
syn keyword icingaDirective contained hostgroup_members hostgroup_name hostgroups icon_image icon_image_alt inherits_parent is_volatile last_notification
syn keyword icingaDirective contained low_flap_threshold max_check_attempts members monday name normal_check_interval notes notes_url
syn keyword icingaDirective contained notification_failure_criteria notification_failure_options notification_interval notification_options
syn keyword icingaDirective contained notification_period notifications_enabled obsess_over_host obsess_over_service pager parallelize_check parents
syn keyword icingaDirective contained passive_checks_enabled process_perf_data register retain_nonstatus_information retain_status_information
syn keyword icingaDirective contained retry_check_interval retry_interval saturday service_description service_notification_commands
syn keyword icingaDirective contained service_notification_options service_notification_period service_notifications_enabled servicegroup_name
syn keyword icingaDirective contained servicegroups stalking_options statusmap_image sunday thursday timeperiod_name tuesday use vrml_image wednesday

syn keyword icingaConfigOption accept_passive_host_checks accept_passive_service_checks additional_freshness_latency admin_email admin_pager
syn keyword icingaConfigOption allow_empty_hostgroup_assignment auto_reschedule_checks auto_rescheduling_interval auto_rescheduling_window
syn keyword icingaConfigOption broker_module cached_host_check_horizon cached_service_check_horizon cfg_dir cfg_file check_external_commands
syn keyword icingaConfigOption check_for_orphaned_hosts check_for_orphaned_services check_host_freshness check_result_path
syn keyword icingaConfigOption check_result_reaper_frequency check_service_freshness child_processes_fork_twice command_check_interval
syn keyword icingaConfigOption command_file config_file daemon_dumps_core date_format debug_file debug_level debug_verbosity
syn keyword icingaConfigOption dump_retained_host_service_states_to_neb enable_embedded_perl enable_environment_macros enable_event_handlers
syn keyword icingaConfigOption enable_flap_detection enable_notifications enable_predictive_host_dependency_checks
syn keyword icingaConfigOption enable_predictive_service_dependency_checks enable_state_based_escalation_ranges event_broker_options
syn keyword icingaConfigOption event_handler_timeout event_profiling_enabled execute_host_checks execute_service_checks
syn keyword icingaConfigOption external_command_buffer_slots free_child_process_memory global_host_event_handler global_service_event_handler
syn keyword icingaConfigOption high_host_flap_threshold high_service_flap_threshold host_check_timeout host_freshness_check_interval
syn keyword icingaConfigOption host_inter_check_delay_method host_perfdata_command host_perfdata_file host_perfdata_file_mode
syn keyword icingaConfigOption host_perfdata_file_processing_command host_perfdata_file_processing_interval host_perfdata_file_template
syn keyword icingaConfigOption host_perfdata_process_empty_results icinga_group icinga_user illegal_macro_output_chars illegal_object_name_chars
syn keyword icingaConfigOption interval_length keep_unknown_macros lock_file log_archive_path log_current_states log_event_handlers
syn keyword icingaConfigOption log_external_commands log_file log_host_retries log_initial_states log_long_plugin_output log_notifications
syn keyword icingaConfigOption log_passive_checks log_rotation_method log_service_retries low_host_flap_threshold low_service_flap_threshold
syn keyword icingaConfigOption max_check_result_file_age max_check_result_list_items max_check_result_reaper_time max_concurrent_checks
syn keyword icingaConfigOption max_debug_file_size max_host_check_spread max_service_check_spread notification_timeout object_cache_file
syn keyword icingaConfigOption obsess_over_hosts obsess_over_services ochp_command ocsp_command ocsp_timeout p1_file passive_host_checks_are_soft
syn keyword icingaConfigOption perfdata_timeout pnp_path precached_object_file process_performance_data resource_file retain_state_information
syn keyword icingaConfigOption retained_contact_host_attribute_mask retained_contact_service_attribute_mask retained_host_attribute_mask
syn keyword icingaConfigOption retained_process_host_attribute_mask retained_process_service_attribute_mask retained_service_attribute_mask
syn keyword icingaConfigOption retention_update_interval service_check_timeout service_check_timeout_state service_freshness_check_interval
syn keyword icingaConfigOption service_inter_check_delay_method service_interleave_factor service_perfdata_command service_perfdata_file
syn keyword icingaConfigOption service_perfdata_file_mode service_perfdata_file_processing_command service_perfdata_file_processing_interval
syn keyword icingaConfigOption service_perfdata_file_template service_perfdata_process_empty_results sleep_time soft_state_dependencies
syn keyword icingaConfigOption stalking_event_handlers_for_hosts stalking_event_handlers_for_services stalking_notifications_for_hosts
syn keyword icingaConfigOption stalking_notifications_for_services state_retention_file status_file status_update_interval sync_retention_file
syn keyword icingaConfigOption syslog_local_facility temp_file temp_path time_change_threshold translate_passive_host_checks
syn keyword icingaConfigOption use_aggressive_host_checking use_daemon_log use_embedded_perl_implicitly use_large_installation_tweaks
syn keyword icingaConfigOption use_regexp_matching use_retained_program_state use_retained_scheduling_info use_syslog use_syslog_local_facility
syn keyword icingaConfigOption use_timezone use_true_regexp_matching

syn keyword icingaConfigOption action_url_target add_notif_num_hard add_notif_num_soft authorization_config_file
syn keyword icingaConfigOption authorized_contactgroup_for_all_hosts authorized_contactgroup_for_all_services
syn keyword icingaConfigOption authorized_contactgroup_for_comments_read_only authorized_contactgroup_for_configuration_information
syn keyword icingaConfigOption authorized_contactgroup_for_downtimes_read_only authorized_contactgroup_for_full_command_resolution
syn keyword icingaConfigOption authorized_contactgroup_for_read_only authorized_contactgroup_for_system_information
syn keyword icingaConfigOption authorized_for_all_host_commands authorized_for_all_hosts authorized_for_all_service_commands
syn keyword icingaConfigOption authorized_for_all_services authorized_for_comments_read_only authorized_for_configuration_information
syn keyword icingaConfigOption authorized_for_downtimes_read_only authorized_for_full_command_resolution authorized_for_read_only
syn keyword icingaConfigOption authorized_for_system_commands authorized_for_system_information cgi_log_archive_path cgi_log_file
syn keyword icingaConfigOption cgi_log_rotation_method color_transparency_index_b color_transparency_index_g color_transparency_index_r
syn keyword icingaConfigOption csv_data_enclosure csv_delimiter default_downtime_duration default_expiring_acknowledgement_duration
syn keyword icingaConfigOption default_expiring_disabled_notifications_duration default_statusmap_layout default_user_name display_status_totals
syn keyword icingaConfigOption enable_splunk_integration enforce_comments_on_actions escape_html_tags exclude_customvar_name
syn keyword icingaConfigOption exclude_customvar_value extinfo_show_child_hosts first_day_of_week highlight_table_rows host_down_sound
syn keyword icingaConfigOption host_unreachable_sound http_charset lock_author_names lowercase_user_name main_config_file max_check_attempts
syn keyword icingaConfigOption normal_sound notes_url_target persistent_ack_comments physical_html_path refresh_rate refresh_type result_limit
syn keyword icingaConfigOption send_ack_notifications service_critical_sound service_unknown_sound service_warning_sound set_expire_ack_by_default
syn keyword icingaConfigOption show_all_services_host_is_authorized_for show_partial_hostgroups show_partial_servicegroups show_tac_header
syn keyword icingaConfigOption show_tac_header_pending standalone_installation showlog_current_states showlog_initial_states splunk_url
syn keyword icingaConfigOption status_show_long_plugin_output statusmap_background_image suppress_maintenance_downtime tab_friendly_titles
syn keyword icingaConfigOption tac_show_only_hard_state url_html_path url_stylesheets_path use_authentication use_logging use_pending_states
syn keyword icingaConfigOption use_ssl_authentication

hi link icingaComment Comment
hi link icingaLineComment Comment
hi link icingaConstant Number
hi link icingaDef Statement
hi link icingaDirective Type
hi link icingaConfigOption Type
hi link icingaCGIConfigOption Type
hi link icingaMacro Macro
hi link icingaString String
hi link icingaSpecial Special
