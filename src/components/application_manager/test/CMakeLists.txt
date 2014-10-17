  set (AM_TEST_DIR ${CMAKE_SOURCE_DIR}/src/components/application_manager/test)
  set (AM_MOCK_DIR ${CMAKE_SOURCE_DIR}/src/components/application_manager/test/mock)

get_property(the_include_dirs DIRECTORY "" PROPERTY INCLUDE_DIRECTORIES)
#replace include for mocking singltone
set(class_to_mock ${CMAKE_SOURCE_DIR}/src/components/application_manager/include)
list(FIND the_include_dirs ${class_to_mock} find_idx)
if(find_idx GREATER -1)
  LIST_REPLACE(the_include_dirs ${find_idx} ${AM_MOCK_DIR}/include)
endif()
set_property(DIRECTORY "" PROPERTY INCLUDE_DIRECTORIES ${the_include_dirs})

  include_directories (
    ${CMAKE_SOURCE_DIR}/src/3rd_party-static/gmock-1.7.0/include
    ${CMAKE_SOURCE_DIR}/src/3rd_party-static/gmock-1.7.0/gtest/include
    ${CMAKE_SOURCE_DIR}/src/components/protocol_handler/include/
    ${JSONCPP_INCLUDE_DIRECTORY}
    ${CMAKE_SOURCE_DIR}/src/components/formatters/include
    ${CMAKE_SOURCE_DIR}/src/components/transport_manager/include
    ${SecurityManagerIncludeDir}
    ${CMAKE_SOURCE_DIR}/src/components/config_profile/include
    ${CMAKE_SOURCE_DIR}/src/components/utils/include/
    ${CMAKE_SOURCE_DIR}/src/components/connection_handler/include/
    ${CMAKE_SOURCE_DIR}/src/components/hmi_message_handler/include
    ${CMAKE_SOURCE_DIR}/src/components/request_watchdog/include
    ${CMAKE_SOURCE_DIR}/src/components/smart_objects/include/
    ${CMAKE_SOURCE_DIR}/src/components/media_manager/include/
    ${CMAKE_SOURCE_DIR}/src/components/time_tester/include
    ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/include/
    ${CMAKE_SOURCE_DIR}/src/components/resumption/include/
    ${MESSAGE_BROKER_INCLUDE_DIRECTORY}
    ${ENCRYPTION_INCLUDE_DIRECTORY}
    ${CMAKE_SOURCE_DIR}/src/components/
    ${CMAKE_BINARY_DIR}/src/components/
    ${CMAKE_SOURCE_DIR}/src/components/dbus/include/
    ${CMAKE_BINARY_DIR}/src/components/policy/src/policy
    ${CMAKE_SOURCE_DIR}
    ${default_media_inc}
    ${LOG4CXX_INCLUDE_DIRECTORY}
  )

  set(testSources
    ${CMAKE_SOURCE_DIR}/src/3rd_party-static/gmock-1.7.0/src/gmock_main.cc
    ${AM_TEST_DIR}/command_impl_test.cc
  )
  set (mockedSources
    ${AM_MOCK_DIR}/src/application_manager_impl.cc
  )

  set (symboliLinkSources

    ${AM_MOCK_DIR}/src/usage_statistics.cc
    ${AM_MOCK_DIR}/src/request_info.cc
    ${AM_MOCK_DIR}/src/message.cc
    ${AM_MOCK_DIR}/src/application_impl.cc
    ${AM_MOCK_DIR}/src/mobile_command_factory.cc
    ${AM_MOCK_DIR}/src/message_helper.cc
    ${AM_MOCK_DIR}/src/hmi_command_factory.cc
    ${AM_MOCK_DIR}/src/hmi_capabilities.cc
    ${AM_MOCK_DIR}/src/application_data_impl.cc
    ${AM_MOCK_DIR}/src/request_controller.cc
    ${AM_MOCK_DIR}/src/resume_ctrl.cpp
    ${AM_MOCK_DIR}/src/mobile_message_handler.cc

    #Policies:
    ${AM_MOCK_DIR}/src/policies/policy_handler.cc
    ${AM_MOCK_DIR}/src/policies/policy_event_observer.cc

    #EVENT_ENGINE
    ${AM_MOCK_DIR}/src/event_engine/event.cc
    ${AM_MOCK_DIR}/src/event_engine/event_dispatcher.cc
    ${AM_MOCK_DIR}/src/event_engine/event_observer.cc

    #MOBILE_COMMANDS_SOURCES
    ${AM_MOCK_DIR}/src/commands/command_impl.cc
    ${AM_MOCK_DIR}/src/commands/command_request_impl.cc
    ${AM_MOCK_DIR}/src/commands/command_response_impl.cc
    ${AM_MOCK_DIR}/src/commands/command_notification_impl.cc
    ${AM_MOCK_DIR}/src/commands/pending.cc
    ${AM_MOCK_DIR}/src/commands/mobile/add_command_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/add_command_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/add_sub_menu_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/add_sub_menu_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/alert_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/alert_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/alert_maneuver_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/alert_maneuver_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/change_registration_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/change_registration_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/create_interaction_choice_set_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/create_interaction_choice_set_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_command_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_command_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_file_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_file_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_interaction_choice_set_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_interaction_choice_set_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_sub_menu_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/delete_sub_menu_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/dial_number_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/end_audio_pass_thru_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/end_audio_pass_thru_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/generic_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/get_dtcs_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/get_dtcs_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/diagnostic_message_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/diagnostic_message_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/get_vehicle_data_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/get_vehicle_data_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/list_files_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/list_files_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_app_interface_unregistered_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_audio_pass_thru_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_button_event_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_button_press_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_driver_distraction_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_hmi_status_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_language_change_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_command_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_permissions_change_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_tbt_client_state_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_vehicle_data_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/perform_audio_pass_thru_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/perform_audio_pass_thru_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/perform_interaction_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/perform_interaction_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/put_file_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/put_file_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/read_did_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/read_did_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/register_app_interface_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/register_app_interface_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/reset_global_properties_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/reset_global_properties_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/scrollable_message_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/scrollable_message_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_display_layout_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_display_layout_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_global_properties_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_global_properties_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_icon_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_icon_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_media_clock_timer_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/set_media_clock_timer_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/show_constant_tbt_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/show_constant_tbt_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/show_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/show_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/slider_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/slider_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/speak_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/speak_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/subscribe_button_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/subscribe_button_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/subscribe_vehicle_data_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/subscribe_vehicle_data_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/unregister_app_interface_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/unregister_app_interface_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/unsubscribe_button_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/unsubscribe_button_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/unsubscribe_vehicle_data_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/unsubscribe_vehicle_data_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/update_turn_list_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/update_turn_list_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/system_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/system_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/send_location_request.cc
    ${AM_MOCK_DIR}/src/commands/mobile/send_location_response.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_keyboard_input_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_touch_event_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_system_request_notification.cc
    ${AM_MOCK_DIR}/src/commands/mobile/on_hash_change_notification.cc

    #HMI_COMMANDS_SOURCES
    ${AM_MOCK_DIR}/src/commands/hmi/request_to_hmi.cc
    ${AM_MOCK_DIR}/src/commands/hmi/response_from_hmi.cc
    ${AM_MOCK_DIR}/src/commands/hmi/request_from_hmi.cc
    ${AM_MOCK_DIR}/src/commands/hmi/response_to_hmi.cc
    ${AM_MOCK_DIR}/src/commands/hmi/notification_to_hmi.cc
    ${AM_MOCK_DIR}/src/commands/hmi/notification_from_hmi.cc
    ${AM_MOCK_DIR}/src/commands/hmi/allow_all_apps_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/allow_all_apps_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/allow_app_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/allow_app_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/update_sdl_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/update_sdl_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/activate_app_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/activate_app_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/get_system_info_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/get_system_info_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_get_list_of_permissions_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_get_list_of_permissions_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_get_user_friendly_message_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_get_user_friendly_message_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_get_status_update_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_get_status_update_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_status_update_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_activate_app_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_activate_app_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/mixing_audio_supported_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/mixing_audio_supported_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/update_app_list_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/update_app_list_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_update_device_list.cc
    ${AM_MOCK_DIR}/src/commands/hmi/update_device_list_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/update_device_list_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_sdl_consent_needed_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_sdl_persistence_complete_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_exit_all_applications_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_exit_application_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_start_device_discovery.cc
    ${AM_MOCK_DIR}/src/commands/hmi/close_popup_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/close_popup_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_app_activated_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/button_get_capabilities_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/button_get_capabilities_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_is_ready_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_is_ready_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_add_command_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_add_command_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_delete_command_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_delete_command_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_change_registration_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_change_registration_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_get_supported_languages_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_get_supported_languages_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_get_language_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_get_language_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_get_capabilities_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_get_capabilities_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_is_ready_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_is_ready_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_change_registration_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_change_registration_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_get_language_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_get_language_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_get_supported_languages_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_get_supported_languages_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_stop_speaking_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_stop_speaking_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_speak_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_speak_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_set_global_properties_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_set_global_properties_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_get_capabilities_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/tts_get_capabilities_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_perform_interaction_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vr_perform_interaction_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_add_command_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_add_command_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_delete_command_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_delete_command_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_add_submenu_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_add_submenu_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_delete_submenu_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_delete_submenu_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_get_supported_languages_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_get_supported_languages_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_get_language_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_get_language_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_get_capabilities_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_get_capabilities_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_change_registration_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_change_registration_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_show_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_show_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_alert_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_alert_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_slider_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_scrollable_message_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_scrollable_message_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_global_properties_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_global_properties_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_slider_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_is_ready_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_is_ready_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_perform_audio_pass_thru_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_perform_audio_pass_thru_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_end_audio_pass_thru_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_end_audio_pass_thru_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_perform_interaction_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_perform_interaction_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_icon_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_icon_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_media_clock_timer_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_media_clock_timer_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_show_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_show_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_is_ready_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_is_ready_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_read_did_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_read_did_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_get_dtcs_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_get_dtcs_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_diagnostic_message_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_diagnostic_message_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_get_vehicle_type_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_get_vehicle_type_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_is_ready_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_is_ready_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_alert_maneuver_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_alert_maneuver_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_update_turn_list_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_update_turn_list_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_show_constant_tbt_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_show_constant_tbt_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_send_location_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_send_location_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_ready_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_system_context_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_device_chosen_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_allow_sdl_functionality_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_app_permission_consent_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_app_registered_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_app_permission_changed_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_app_unregistered_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_driver_distraction_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_ignition_cycle_over_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_system_info_changed_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_file_removed_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_play_tone_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_tts_started_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_tts_stopped_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vr_started_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vr_stopped_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vr_command_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_ui_command_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_navi_tbt_client_state_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_app_deactivated_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_tts_language_change_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vr_language_change_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_ui_language_change_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_button_event_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_button_press_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_find_applications.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_ui_keyboard_input_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_ui_touch_event_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_ui_reset_timeout_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_start_stream_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_start_stream_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_stop_stream_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_stop_stream_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_audio_start_stream_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_audio_start_stream_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_audio_stop_stream_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/navi_audio_stop_stream_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_system_request_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_put_file_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_resume_audio_source_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_sdl_close_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_display_layout_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/ui_set_display_layout_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_record_start_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/add_statistics_info_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_system_error_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/basic_communication_system_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/basic_communication_system_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_policy_update.cc
    ${AM_MOCK_DIR}/src/commands/hmi/sdl_policy_update_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_received_policy_update.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_policy_update.cc
    ${AM_MOCK_DIR}/src/commands/hmi/get_urls.cc
    ${AM_MOCK_DIR}/src/commands/hmi/get_urls_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_device_state_changed_notification.cc

    #HMI_COMMANDS_SOURCES_JSON
    ${AM_MOCK_DIR}/src/commands/hmi/vi_get_vehicle_data_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_get_vehicle_data_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_subscribe_vehicle_data_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_subscribe_vehicle_data_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_unsubscribe_vehicle_data_request.cc
    ${AM_MOCK_DIR}/src/commands/hmi/vi_unsubscribe_vehicle_data_response.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_vehicle_data_notification.cc

    #HMI_COMMANDS_SOURCES_DBUS
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_gps_data_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_speed_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_rpm_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_fuel_level_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_fuel_level_state_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_instant_fuel_consumption_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_external_temperature_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_vin_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_prndl_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_tire_pressure_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_odometer_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_belt_status_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_body_information_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_device_status_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_driver_braking_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_wiper_status_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_head_lamp_status_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_engine_torque_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_acc_pedal_position_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_steering_wheel_angle_notification.cc
    ${AM_MOCK_DIR}/src/commands/hmi/on_vi_my_key_notification.cc
  )

  if (EXTENDED_POLICY_FLAG)
    include_directories(
      ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/policy_table/table_struct_ext
    )
    list(APPEND symboliLinkSources
      ${AM_MOCK_DIR}/src/policies/pt_exchange_handler_ext.cc
    )
  else ()
    include_directories(
      ${CMAKE_SOURCE_DIR}/src/components/policy/src/policy/policy_table/table_struct
    )
    list(APPEND symboliLinkSources
      ${AM_MOCK_DIR}/src/policies/pt_exchange_handler_impl.cc
      ${AM_MOCK_DIR}/src/policies/policy_retry_sequence.cc
    )
  endif ()

  set(testLibraries
    gmock
    gtest
    gmock
    gmock_main
    UsageStatistics
    dl
    ProtocolLibrary
    )

  set(test_exec_libraries
    HMI_API
    MOBILE_API
    v4_protocol_v1_2_no_extra
    SmartObjects
    formatters
    ProtocolHandler
    connectionHandler
    HMIMessageHandler
    Utils
    jsoncpp
    ConfigProfile
    MediaManager
    Resumption
  )

IF(${CMAKE_SYSTEM_NAME} MATCHES "QNX")
  list(REMOVE_ITEM test_exec_libraries dl)
endif()

if(ENABLE_LOG)
  list(APPEND LIBRARIES log4cxx -L${LOG4CXX_LIBS_DIRECTORY})
  list(APPEND LIBRARIES apr-1 -L${APR_LIBS_DIRECTORY})
  list(APPEND LIBRARIES aprutil-1 -L${APR_UTIL_LIBS_DIRECTORY})
  list(APPEND LIBRARIES expat -L${EXPAT_LIBS_DIRECTORY})
endif()

  add_library("ApplicationManagerTest" ${testSources} ${mockedSources} ${symboliLinkSources} )
  target_link_libraries("ApplicationManagerTest" ${testLibraries})

  add_executable(application_manager_test ${testSources})
  target_link_libraries(application_manager_test ApplicationManagerTest ${testLibraries} ${test_exec_libraries})
