idf_component_register(SRCS "esp32-main.c" "appmqtt.c" "appwebserver.c" "appota.c" "appstate.c" "appwifi.c" "appfilesystem.c" "appusbhost.c" "arty_driver.c" "frozen/frozen.c" "ssd1306.c" "appuart.c" "ftdi.c" "jtag.c"
	INCLUDE_DIRS "." "./frozen" 
                       EMBED_TXTFILES ${project_dir}/ca/caroot.pem ${project_dir}/ca/cakey.pem)
