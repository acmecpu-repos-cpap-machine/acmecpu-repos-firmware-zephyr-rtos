
set(COMPONENT_SRCS 	"host_cmds.c"
#					"m2m_frame.c"
					"host_cmds_send_recv.c"
					"host_cmds_req.c"
					"host_cmds_devinf.c"
					"host_cmds_battery.c"
					"host_cmds_blower.c"
					"host_cmds_dfu.c"
					"host_cmds_stepper.c"
					"host_cmds_settings.c"
					"host_cmds_sensor.c"
					"host_cmds_html_server.c"
					"host_cmds_wifi.c"
					"host_cmds_net.c"
					"host_cmds_fw_update.c"
								)
								
set(COMPONENT_ADD_INCLUDEDIRS ". include")

set(COMPONENT_REQUIRES 	generic_linked_lists
						comm_wifi
						driver
						app_net
						lib
						app_update
						)

register_component()
