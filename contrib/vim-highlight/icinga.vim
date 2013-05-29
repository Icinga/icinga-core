" Vim syntax file
" Filename:     icinga.vim
" Language:     icinga template object configuration file (based on nagios.vim by the following authors)
" Author:       Elan Ruusamäe <glen@pld-linux.org>
" Author:       Lance Albertson <ramereth@gentoo.org>
" Author:       Ava Arachne Jarvis <ajar@katanalynx.dyndns.org>
" Maintainer:   Lars Engels <lars.engels@0x20.net>
" Last Change:  2013-05-07 16:19

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

syn keyword icingaDirective contained name register use
syn keyword icingaDirective contained active_checks_enabled address alias check_command
syn keyword icingaDirective contained check_freshness check_period checks_enabled check_interval retry_interval
syn keyword icingaDirective contained command_line command_name
syn keyword icingaDirective contained contact_groups contact_name contactgroup_name contacts
syn keyword icingaDirective contained dependent_host_name dependent_service_description
syn keyword icingaDirective contained email event_handler event_handler_enabled
syn keyword icingaDirective contained execution_failure_criteria first_notification execution_failure_options
syn keyword icingaDirective contained flap_detection_enabled freshness_threshold failure_prediction_enabled
syn keyword icingaDirective contained friday high_flap_threshold host_name
syn keyword icingaDirective contained host_notification_commands
syn keyword icingaDirective contained host_notification_options hostgroup_members
syn keyword icingaDirective contained host_notification_period hostgroup_name servicegroup_name hostgroups servicegroups
syn keyword icingaDirective contained is_volatile last_notification
syn keyword icingaDirective contained low_flap_threshold max_check_attempts
syn keyword icingaDirective contained members monday normal_check_interval
syn keyword icingaDirective contained notification_failure_criteria notification_failure_options
syn keyword icingaDirective contained notification_interval notification_options
syn keyword icingaDirective contained notification_period notifications_enabled
syn keyword icingaDirective contained obsess_over_service pager parallelize_check
syn keyword icingaDirective contained parents passive_checks_enabled
syn keyword icingaDirective contained process_perf_data retain_nonstatus_information
syn keyword icingaDirective contained retain_status_information retry_check_interval
syn keyword icingaDirective contained saturday service_description
syn keyword icingaDirective contained service_notification_commands
syn keyword icingaDirective contained service_notification_options
syn keyword icingaDirective contained service_notification_period stalking_options
syn keyword icingaDirective contained sunday thursday timeperiod_name tuesday wednesday
syn keyword icingaDirective contained icon_image icon_image_alt vrml_image statusmap_image
syn keyword icingaDirective contained notes notes_url action_url 2d_coords 3d_coords obsess_over_host inherits_parent
syn keyword icingaDirective contained can_submit_commands host_notifications_enabled service_notifications_enabled

syn keyword icingaConfigOption accept_passive_host_checks accept_passive_service_checks additional_freshness_latency
syn keyword icingaConfigOption admin_email admin_pager allow_empty_hostgroup_assignment auto_reschedule_checks
syn keyword icingaConfigOption auto_rescheduling_interval auto_rescheduling_window broker_module cached_host_check_horizon
syn keyword icingaConfigOption cached_service_check_horizon cfg_dir cfg_file check_external_commands check_for_orphaned_hosts
syn keyword icingaConfigOption check_for_orphaned_services check_host_freshness check_result_path check_result_reaper_frequency
syn keyword icingaConfigOption check_service_freshness child_processes_fork_twice command_check_interval command_file
syn keyword icingaConfigOption config_file daemon_dumps_core date_format debug_file debug_level debug_verbosity
syn keyword icingaConfigOption dump_retained_host_service_states_to_neb enable_embedded_perl enable_environment_macros
syn keyword icingaConfigOption enable_event_handlers enable_flap_detection enable_notifications
syn keyword icingaConfigOption enable_predictive_host_dependency_checks enable_predictive_service_dependency_checks
syn keyword icingaConfigOption enable_state_based_escalation_ranges event_broker_options event_handler_timeout
syn keyword icingaConfigOption event_profiling_enabled execute_host_checks execute_service_checks
syn keyword icingaConfigOption external_command_buffer_slots free_child_process_memory global_host_event_handler
syn keyword icingaConfigOption global_service_event_handler high_host_flap_threshold high_service_flap_threshold
syn keyword icingaConfigOption host_check_timeout host_freshness_check_interval host_inter_check_delay_method
syn keyword icingaConfigOption host_perfdata_command host_perfdata_file host_perfdata_file_mode host_perfdata_file_processing_command
syn keyword icingaConfigOption host_perfdata_file_processing_interval host_perfdata_file_template host_perfdata_process_empty_results
syn keyword icingaConfigOption icinga_group icinga_user illegal_macro_output_chars illegal_object_name_chars interval_length
syn keyword icingaConfigOption keep_unknown_macros lock_file log_archive_path log_current_states log_event_handlers
syn keyword icingaConfigOption log_external_commands log_file log_host_retries log_initial_states log_long_plugin_output
syn keyword icingaConfigOption log_notifications log_passive_checks log_rotation_method log_service_retries low_host_flap_threshold
syn keyword icingaConfigOption low_service_flap_threshold max_check_result_file_age max_check_result_list_items
syn keyword icingaConfigOption max_check_result_reaper_time max_concurrent_checks max_debug_file_size max_host_check_spread
syn keyword icingaConfigOption max_service_check_spread notification_timeout object_cache_file obsess_over_hosts
syn keyword icingaConfigOption obsess_over_services ochp_command ocsp_command ocsp_timeout p1_file passive_host_checks_are_soft
syn keyword icingaConfigOption perfdata_timeout pnp_path precached_object_file process_performance_data resource_file
syn keyword icingaConfigOption retain_state_information retained_contact_host_attribute_mask retained_contact_service_attribute_mask
syn keyword icingaConfigOption retained_host_attribute_mask retained_process_host_attribute_mask retained_process_service_attribute_mask
syn keyword icingaConfigOption retained_service_attribute_mask retention_update_interval service_check_timeout service_check_timeout_state
syn keyword icingaConfigOption service_freshness_check_interval service_inter_check_delay_method service_interleave_factor
syn keyword icingaConfigOption service_perfdata_command service_perfdata_file service_perfdata_file_mode
syn keyword icingaConfigOption service_perfdata_file_processing_command service_perfdata_file_processing_interval service_perfdata_file_template
syn keyword icingaConfigOption service_perfdata_process_empty_results sleep_time soft_state_dependencies stalking_event_handlers_for_hosts
syn keyword icingaConfigOption stalking_event_handlers_for_services stalking_notifications_for_hosts stalking_notifications_for_services
syn keyword icingaConfigOption state_retention_file status_file status_update_interval sync_retention_file syslog_local_facility temp_file
syn keyword icingaConfigOption temp_path time_change_threshold translate_passive_host_checks use_aggressive_host_checking use_daemon_log
syn keyword icingaConfigOption use_embedded_perl_implicitly use_large_installation_tweaks use_regexp_matching use_retained_program_state
syn keyword icingaConfigOption use_retained_scheduling_info use_syslog use_syslog_local_facility use_timezone use_true_regexp_matching

syn keyword icingaCGIConfigOption action_url_target add_notif_num_hard add_notif_num_soft authorization_config_file
syn keyword icingaCGIConfigOption authorized_contactgroup_for_all_hosts authorized_contactgroup_for_all_services
syn keyword icingaCGIConfigOption authorized_contactgroup_for_comments_read_only authorized_contactgroup_for_configuration_information
syn keyword icingaCGIConfigOption authorized_contactgroup_for_downtimes_read_only authorized_contactgroup_for_full_command_resolution
syn keyword icingaCGIConfigOption authorized_contactgroup_for_read_only authorized_contactgroup_for_system_information
syn keyword icingaCGIConfigOption authorized_for_all_host_commands authorized_for_all_hosts authorized_for_all_service_commands
syn keyword icingaCGIConfigOption authorized_for_all_services authorized_for_comments_read_only authorized_for_configuration_information
syn keyword icingaCGIConfigOption authorized_for_downtimes_read_only authorized_for_full_command_resolution authorized_for_read_only
syn keyword icingaCGIConfigOption authorized_for_system_commands authorized_for_system_information cgi_log_archive_path cgi_log_file
syn keyword icingaCGIConfigOption cgi_log_rotation_method color_transparency_index_b color_transparency_index_g color_transparency_index_r
syn keyword icingaCGIConfigOption csv_data_enclosure csv_delimiter default_downtime_duration default_expiring_acknowledgement_duration
syn keyword icingaCGIConfigOption default_expiring_disabled_notifications_duration default_statusmap_layout default_user_name
syn keyword icingaCGIConfigOption display_status_totals enable_splunk_integration enforce_comments_on_actions escape_html_tags
syn keyword icingaCGIConfigOption w_child_hosts first_day_of_week highlight_table_rows host_down_sound host_unreachable_sound http_charset
syn keyword icingaCGIConfigOption lock_author_names lowercase_user_name main_config_file max_check_attempts normal_sound
syn keyword icingaCGIConfigOption notes_url_target persistent_ack_comments physical_html_path refresh_rate refresh_type result_limit
syn keyword icingaCGIConfigOption service_critical_sound service_unknown_sound service_warning_sound
syn keyword icingaCGIConfigOption show_all_services_host_is_authorized_for show_partial_hostgroups show_tac_header show_tac_header_pending
syn keyword icingaCGIConfigOption showlog_current_states showlog_initial_states splunk_url status_show_long_plugin_output
syn keyword icingaCGIConfigOption statusmap_background_image suppress_maintenance_downtime tab_friendly_titles tac_show_only_hard_state
syn keyword icingaCGIConfigOption url_html_path url_stylesheets_path use_authentication use_logging use_pending_states
syn keyword icingaCGIConfigOption use_ssl_authentication

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
